#!/usr/bin/env python3

from taiseilib.common import (
    add_common_args,
    run_main,
)

from pathlib import Path
from tempfile import TemporaryDirectory

import argparse
import subprocess

BASISU_TAISEI_ID = 0x52656900
BASISU_TAISEI_CHANNELS_MASK = 0x3
BASISU_TAISEI_SRGB = (1 << 2)
BASISU_TAISEI_NORMALMAP = (1 << 3)


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


def preprocess(args, tempdir):
    cmd = [
        'convert',
        '-verbose',
        args.input
    ]

    if args.channels == 'rgba':
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

    if cmd[-1] != args.input or args.input.suffix != '.png':
        dst = tempdir / 'preprocessed.png'
        cmd += [dst]
        run(args, cmd)
        return dst

    return args.input


def process(args):
    with TemporaryDirectory() as tempdir:
        tempdir = Path(tempdir)
        img = preprocess(args, tempdir)

        cmd = [
            args.basisu,
            '-file', img,
            '-output_file', args.output,
        ]

        taisei_flags = ('r', 'rg', 'rgb', 'rgba').index(args.channels)
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

        if args.channels == 'rg':
            cmd += ['-separate_rg_to_color_alpha']

        if args.channels != 'rgba':
            cmd += ['-no_alpha']

        cmd += [
            '-userdata0', str(taisei_flags),
            '-userdata1', str(BASISU_TAISEI_ID)
        ]

        if args.uastc:
            cmd += ['-uastc']

            profiles = {
                'slow': [
                    '-uastc_level', '3',
                    '-uastc_rdo_q', '4',
                ],
                'fast': [
                    '-uastc_level', '1',
                ],
                'incredibly_slow': [
                    '-uastc_level', '4',
                    '-uastc_rdo_q', '4',
                ],
            }
        else:
            profiles = {
                'slow': [
                    '-comp_level', '4',
                    '-q', '255'
                ],
                'fast': [],
                'incredibly_slow': [
                    '-comp_level', '5',
                    '-max_endpoints', '16128',
                    '-max_selectors', '16128',
                    '-no_selector_rdo',
                    '-no_endpoint_rdo',
                ],
            }

        cmd += profiles[args.profile]

        """
        if args.basisu_args is not None:
            cmd += args.basisu_args
        """

        run(args, cmd)

        if args.uastc and not args.dry_run:
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
        help='the output .basis file',
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

    channels_choices = ('r', 'rg', 'rgb', 'rgba')
    channels_default = 'rgb'

    parser.add_argument('-c', '--channels',
        help=f'which input channels must be preserved (default: {channels_default})',
        default=channels_default,
        choices=channels_choices,
    )

    for ch in channels_choices:
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
        help='encode to UASTC: large size, high quality',
        action='store_true',
    )

    parser.add_argument('--dry-run',
        help='do nothing, print commands that would have been run',
        action='store_true',
    )

    # FIXME this doesn't work properly (goddamn argparse)
    """
    parser.add_argument('--basisu-args',
        help='pass remaining args through to the encoder',
        nargs='*',
    )
    """

    args = parser.parse_args(args[1:])

    if args.channels == 'rgba' and args.alphamap and args.alphamap_path is None:
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
        args.output = args.input.with_suffix('.basis')

    if not args.srgb:
        args.srgb_sampling = args.force_srgb_sampling

    # print(args)
    process(args)


if __name__ == '__main__':
    run_main(main)
