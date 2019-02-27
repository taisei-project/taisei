#!/usr/bin/env python3

from taiseilib.common import (
    add_common_args,
    run_main,
    update_text_file,
    write_depfile,
)

from pathlib import (
    Path,
)

import argparse
import shutil


def create_fs(args, deps):
    target_dir = args.output.resolve()
    res_dir = Path(args.rootdir) / 'resources'

    extfilter = [
        '.bgm',
        '.build',
        '.glsl',
        '.glslh',
        '.ogg',
        '.sfx',
    ]

    mapping = {}

    def map_files(src, dest, ignore):
        for p in src.glob('**/*'):
            if p.is_dir() or ignore(p):
                continue

            deps.append(p)
            relpath = p.relative_to(src)
            outpath = dest / relpath
            mapping[outpath] = p

    for pkg in sorted(res_dir.glob('*.pkgdir')):
        map_files(pkg, target_dir, lambda p: p.suffix in extfilter)

    map_files(args.shader_dir, target_dir / 'shader', lambda p: p.suffix != '.glsl')

    if args.print:
        for k in sorted(mapping):
            print(k, '<--', mapping[k])
    else:
        if target_dir.is_dir():
            shutil.rmtree(str(target_dir))

        target_dir.mkdir(exist_ok=True)

        for k in sorted(mapping):
            k.parent.mkdir(exist_ok=True, parents=True)
            shutil.copy2(mapping[k], k)


def main(args):
    parser = argparse.ArgumentParser(description='Populate a directory with assets for the Emscripten build', prog=args[0])

    parser.add_argument('output',
        help='the output directory; it will be overwritten',
        type=Path,
    )

    parser.add_argument('shader_dir',
        help='path to prebuilt GLSL ES shaders',
        type=Path,
    )

    parser.add_argument('--stupid-useless-file',
        help='write a stupid useless file to make Meson happy',
        type=Path,
    )

    parser.add_argument('--print',
        help='don\'t copy anything, just print out the mapping',
        action='store_true',
    )

    add_common_args(parser, depfile=True)
    args = parser.parse_args(args[1:])

    deps = [str(Path(__file__).resolve())]
    create_fs(args, deps)

    if args.depfile is not None:
        write_depfile(args.depfile, args.output, deps)

    if args.stupid_useless_file is not None:
        update_text_file(args.stupid_useless_file, '')


if __name__ == '__main__':
    run_main(main)

