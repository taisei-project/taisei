#!/usr/bin/env python3

from taiseilib.common import (
    add_common_args,
    run_main,
)

from pathlib import Path
from tempfile import TemporaryDirectory
from dataclasses import dataclass

import argparse
import subprocess
import sys
import shlex

import numpy as np
from PIL import Image

BASISU_TAISEI_ID            = 0x52656900
BASISU_TAISEI_CHANNELS      = ('r', 'rg', 'rgb', 'rgba', 'gray-alpha')
BASISU_TAISEI_CHANNELS_MASK = 0x3
BASISU_TAISEI_SRGB          = (BASISU_TAISEI_CHANNELS_MASK + 1) << 0
BASISU_TAISEI_NORMALMAP     = (BASISU_TAISEI_CHANNELS_MASK + 1) << 1
BASISU_TAISEI_GRAYALPHA     = (BASISU_TAISEI_CHANNELS_MASK + 1) << 2


class MkbasisInputError(RuntimeError):
    pass


@dataclass
class Cubemap:
    px: Path
    nx: Path
    py: Path
    ny: Path
    pz: Path
    nz: Path


def channels_have_alpha(chans):
    return chans in ('rgba', 'gray-alpha')


def format_cmd(cmd):
    s = []

    for a in [str(x) for x in cmd]:
        if repr(a) != f"'{a}'" or ' ' in a:
            a = repr(a)
        s.append(a)

    return ' '.join(s)


def run(args, cmd):
    print('RUN:  ' + format_cmd(cmd))
    if not args.dry_run:
        subprocess.check_call(cmd)


def image_size(img_path):
    o = subprocess.check_output(['convert', img_path, '-ping', '-print', '%wx%h', 'null:'])
    return tuple(int(d) for d in o.strip().decode('utf8').split('x'))


def preprocess(args, tempdir):
    cmd = [
        'convert',
        '-verbose',
        args.input
    ] + args.preprocess

    if args.channels == 'gray-alpha':
        cmd += [
            '-colorspace', 'gray'
        ]

    if channels_have_alpha(args.channels):
        if args.multiply_alpha:
            cmd += [
                '(',
                    '+clone',
                    '-background', 'black',
                    '-alpha', 'remove',
                ')',
                '+swap',
                '-compose', 'copy_opacity',
                '-composite',
            ]

        if args.alphamap:
            cmd += [
                '(',
                    '+clone',
                    '-channel', 'a',
                    '-separate',
                    args.alphamap_path,
                    '-colorspace', 'gray',
                    '-compose', 'multiply',
                    '-composite',
                ')',
                '-compose', 'copy_opacity',
                '-composite',
            ]
    elif args.channels == 'r':
        cmd += [
            '-alpha', 'off',
            '-channel', 'r',
            '-separate',
            '-colorspace', 'gray'
        ]

    cmd += args.preprocess_late

    if cmd[-1] != args.input or args.input.suffix.lower() not in ('.png', '.jpg'):
        dst = tempdir / 'preprocessed.png'
        cmd += [dst]
        run(args, cmd)

        if args.normal:
            dst = renormalize(args, dst, tempdir)

        return dst

    return args.input


def equirect_to_cubemap(args, equirect, width, height, cube_side, tempdir):
    import py360convert

    tempdir = tempdir / 'cubemap'
    tempdir.mkdir()

    with Image.open(equirect) as equirect_img:
        equirect_array = np.array(equirect_img)

    # HACK: py360convert flips these faces for some reason; apply corrections
    transforms = {
        'U': np.flipud,
        'R': np.fliplr,
        'B': np.fliplr,
    }

    faces = {}

    for face, array in py360convert.e2c(equirect_array, cube_side, cube_format='dict').items():
        with Image.fromarray(transforms.get(face, lambda x: x)(array)) as img:
            faces[face] = tempdir / (face + equirect.suffix)
            img.save(faces[face])

    cubemap = Cubemap(
        px=faces['R'],
        nx=faces['L'],
        py=faces['U'],
        ny=faces['D'],
        pz=faces['F'],
        nz=faces['B'],
    )

    return cubemap


def renormalize(args, normalmap, tempdir):
    output = tempdir / 'renormalized.png'
    print(f'RENORM:  {normalmap} --> {output}')

    if args.dry_run:
        return output

    with Image.open(normalmap) as normalmap_img:
        if normalmap_img.mode != 'RGB':
            normalmap_img = normalmap_img.convert('RGB')
        a = np.array(normalmap_img)

    w, h, chans = a.shape
    assert chans == 3
    a = a.reshape(w * h, chans)
    a = (a / 127.5 - 1).T
    a /= np.linalg.norm(a, axis=0)
    a = np.round((a.T + 1) * 127.5)
    a = a.reshape(w, h, chans)
    a = a.astype('uint8')

    with Image.fromarray(a) as img:
        img.save(output)

    return output


def process(args):
    with TemporaryDirectory() as tempdir:
        tempdir = Path(tempdir)
        img = preprocess(args, tempdir)
        width, height = image_size(img)
        cubemap = None

        if args.equirect_cubemap:
            if width != 2 * height or height % 8 != 0:
                raise MkbasisInputError(
                    f'bad equirectangular map dimensions ({width}x{height}): '
                     'must be multiples of 8 and have 2:1 aspect ratio')
            cubemap = equirect_to_cubemap(args, img, width, height, height // 2, tempdir)
        elif width % 4 != 0 or height % 4 != 0:
            raise MkbasisInputError(f'image dimensions are not multiples of 4 ({width}x{height})')

        basis_output = args.output
        zst_output = None

        if basis_output.suffix == '.zst':
            zst_output = basis_output
            basis_output = tempdir / basis_output.with_suffix('').name

        cmd = [
            args.basisu,
            '-output_file', basis_output,
        ]

        if cubemap is None:
            cmd += [
                '-file', img
            ]
        else:
            cmd += [
                '-tex_type', 'cubemap',
                '-file', cubemap.px,
                '-file', cubemap.nx,
                '-file', cubemap.py,
                '-file', cubemap.ny,
                '-file', cubemap.pz,
                '-file', cubemap.nz,
            ]

        if args.channels == 'gray-alpha':
            taisei_flags = BASISU_TAISEI_CHANNELS.index('rg') | BASISU_TAISEI_GRAYALPHA
        else:
            taisei_flags = BASISU_TAISEI_CHANNELS.index(args.channels)
            assert taisei_flags == taisei_flags & BASISU_TAISEI_CHANNELS_MASK

        if args.y_flip:
            cmd += ['-y_flip']

        if args.normal:
            taisei_flags |= BASISU_TAISEI_NORMALMAP
            cmd += ['-normal_map']

        if args.srgb_sampling:
            taisei_flags |= BASISU_TAISEI_SRGB

        if not args.srgb:
            cmd += ['-linear']

        if args.mipmaps:
            cmd += ['-mipmap']

            if args.srgb:
                cmd += ['-mip_srgb']
            else:
                cmd += ['-mip_linear']

            if args.normal:
                cmd += ['-mip_scale', '0.5']

            if cubemap is not None:
                # TODO: expose this as a separate setting?
                cmd += ['-mip_clamp']

        if args.channels == 'rg':
            cmd += ['-separate_rg_to_color_alpha']

        if not channels_have_alpha(args.channels):
            cmd += ['-no_alpha']

        cmd += [
            '-userdata0', str(taisei_flags),
            '-userdata1', str(BASISU_TAISEI_ID)
        ]

        if args.uastc:
            cmd += [
                '-uastc',
            ]

            if args.uastc_rdo > 0:
                cmd += ['-uastc_rdo_l', str(args.uastc_rdo)]

            profiles = {
                'slow': [
                    '-mip_slow',
                    '-uastc_level', '3',
                ],
                'fast': [
                    '-mip_fast',
                    '-uastc_level', '1',
                ],
                'incredibly_slow': [
                    '-mip_slow',
                    '-uastc_level', '4',
                    '-uastc_rdo_d', '8192',
                ],
            }
        else:
            profiles = {
                'slow': [
                    '-mip_slow',
                    # FIXME: raised from 4 due to
                    # https://github.com/BinomialLLC/basis_universal/issues/205
                    '-comp_level', '5',
                    '-q', '255'
                ],
                'fast': [
                    '-mip_fast',
                ],
                'incredibly_slow': [
                    '-mip_slow',
                    '-comp_level', '6',
                    '-max_endpoints', '16128',
                    '-max_selectors', '16128',
                    '-no_selector_rdo',
                    '-no_endpoint_rdo',
                ],
            }

        cmd += profiles[args.profile]
        cmd += args.basisu_args

        run(args, cmd)

        if zst_output:
            cmd = [
                'zstd',
                '-v',
                '-f',
                '--ultra',
                '-22',
                basis_output,
                '-o', zst_output,
            ]
            run(args, cmd)

        if args.uastc and not args.dry_run and not zst_output:
            print('\nNOTE: UASTC textures must be additionally compressed with a general-purpose lossless algorithm!')


def main(args):
    parser = argparse.ArgumentParser(
        description='Generate a Basis Universal compressed texture from image.',
        prog=args[0]
    )

    def make_action(func):
        class ActionWrapper(argparse.Action):
            def __call__(self, parser, namespace, values, option_string):
                return func(parser, namespace, values, option_string)
        return ActionWrapper

    def normal(parser, namespace, *a):
        namespace.channels = 'rg'
        namespace.normal = True
        namespace.srgb = False

    image_suffixes = ('.webp', '.png')

    parser.add_argument('input',
        help='the input image file',
        type=Path,
    )

    parser.add_argument('-o', '--output',
        help='the output .basis file; if ends in .zst it will be compressed with Zstandard',
        type=Path,
    )

    parser.add_argument('--basisu',
        help='Basis Universal encoder command (default: basisu)',
        metavar='COMMAND',
        default='basisu',
    )

    parser.add_argument('--mipmaps',
        dest='mipmaps',
        help='generate mipmaps (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--no-mipmaps',
        dest='mipmaps',
        help="don't generate mipmaps",
        action='store_false',
        default=True,
    )

    parser.add_argument('--srgb',
        dest='srgb',
        help='treat input as sRGB encoded (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--linear',
        dest='srgb',
        help='treat input as linear data, disable sRGB sampling',
        action='store_false',
    )

    parser.add_argument('--srgb-sampling',
        dest='srgb_sampling',
        help='enable sRGB decoding when sampling texture; ignored unless --srgb is active (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--no-srgb-sampling',
        dest='srgb_sampling',
        help='disable sRGB decoding when sampling texture; ignored unless --srgb is active',
        action='store_false',
    )

    parser.add_argument('--force-srgb-sampling',
        help="force sRGB decoding of linear data when sampling; ignored unless --linear is active (you probably don't want this!)",
        action='store_true',
    )

    channels_default = 'rgb'

    parser.add_argument('-c', '--channels',
        help=f'which input channels must be preserved (default: {channels_default})',
        default=channels_default,
        choices=BASISU_TAISEI_CHANNELS,
    )

    for ch in BASISU_TAISEI_CHANNELS:
        parser.add_argument(f'--{ch}',
            dest='channels',
            help=f'alias for --channels={ch}{" (default)" if ch == channels_default else ""}',
            action='store_const',
            const=ch,
        )

    parser.add_argument('--normal',
        help='treat texture as a tangent space normalmap; implies --linear --channels=rg',
        action=make_action(normal),
        nargs=0,
    )

    parser.add_argument('--multiply-alpha',
        dest='multiply_alpha',
        help='premultiply color channels with alpha; ignored unless --channels=rgba (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--no-multiply-alpha',
        dest='multiply_alpha',
        help="don't premultiply color channels with alpha; ignored unless --channels=rgba",
        action='store_false',
    )

    parser.add_argument('--alphamap',
        dest='alphamap',
        help='multiply alpha channel with a custom grayscale image; done after alpha-premultiplication; ignored unless --channels=rgba (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--no-alphamap',
        dest='alphamap',
        help="don't multiply alpha channel with a custom grayscale image; ignored unless --channels=rgba",
        action='store_false',
    )

    parser.add_argument('--alphamap-path',
        help=f'path to alphamap image; defaults to {{input_basename}}.alphamap.{{{",".join(x[1:] for x in image_suffixes)}}} if exists',
        default=None,
        type=Path,
    )

    parser.add_argument('--y-flip',
        dest='y_flip',
        help='flip the texture vertically (default)',
        action='store_true',
        default=True,
    )

    parser.add_argument('--no-y-flip',
        dest='y_flip',
        help="don't flip the texture vertically",
        action='store_false',
        default=True,
    )

    parser.add_argument('--slow',
        dest='profile',
        help='compress slowly, emphasize quality (default)',
        action='store_const',
        const='slow',
        default='slow',
    )

    parser.add_argument('--fast',
        dest='profile',
        help='compress much faster, significantly lower quality',
        action='store_const',
        const='fast',
    )

    parser.add_argument('--incredibly-slow',
        dest='profile',
        help='takes forever, maximum quality',
        action='store_const',
        const='incredibly_slow',
    )

    parser.add_argument('--etc1s',
        dest='uastc',
        help='encode to ETC1S: small size, low/mediocre quality (default)',
        action='store_false',
        default=False,
    )

    parser.add_argument('--uastc',
        dest='uastc',
        help='encode to UASTC: large size, high quality; will output to .basis.zst by default',
        action='store_true',
    )

    parser.add_argument('--uastc-rdo',
        dest='uastc_rdo',
        help='enable Rate Distortion Optimization to improve LZ compression; larger value = worse quality/better compression; try 0.25 - 5.0 (default: 1.0)',
        default=1.0,
        metavar='LEVEL',
        type=float
    )

    parser.add_argument('--equirect-cubemap',
        dest='equirect_cubemap',
        help='treat input as an equirectangular map and convert it into a cubemap',
        action='store_true',
    )

    parser.add_argument('--preprocess',
        dest='preprocess',
        metavar='IMAGEMAGICK_ARGS',
        help='preprocess input with these ImageMagick commands before doing anything',
        type=shlex.split,
        default=[],
    )

    parser.add_argument('--preprocess-late',
        dest='preprocess_late',
        metavar='IMAGEMAGICK_ARGS',
        help='preprocess input with these ImageMagick commands after applying internal preprocessing',
        type=shlex.split,
        default=[],
    )

    parser.add_argument('--basisu-args',
        dest='basisu_args',
        help='pass arguments through to the encoder',
        type=shlex.split,
        default=[],
    )

    parser.add_argument('--dry-run',
        help='do nothing, print commands that would have been run',
        action='store_true',
    )


    args = parser.parse_args(args[1:])

    if channels_have_alpha(args.channels) and args.alphamap and args.alphamap_path is None:
        try:
            for s in image_suffixes:
                p = args.input.with_suffix(f'.alphamap{s}')
                if p.is_file():
                    args.alphamap_path = p
                    break
        except ValueError:
            pass

        args.alphamap = args.alphamap_path is not None

    if args.output is None:
        suffix = '.basis'
        if args.uastc:
            suffix += '.zst'
        args.output = args.input.with_suffix(suffix)

    if not args.srgb:
        args.srgb_sampling = args.force_srgb_sampling

    if args.normal and args.uastc:
        args.channels = 'rgb'

    # print(args)
    try:
        process(args)
    except MkbasisInputError as e:
        print(f'{args.input}: {str(e)}', file=sys.stderr)


if __name__ == '__main__':
    run_main(main)
