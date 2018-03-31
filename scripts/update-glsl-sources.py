#!/usr/bin/python3

from taiseilib.common import (
    run_main,
    update_text_file,
    add_common_args,
)

import re


meson_template = '''
glsl_files += files(
# @begin glsl
# @end glsl
)

subdirs = [
# @begin subdirs
# @end subdirs
]

foreach sd : subdirs
    subdir(sd)
endforeach
'''

re_glsl = re.compile(r'(.*?# @begin glsl\s*?\n)(.*?)(# @end glsl\s*?\n.*)', re.DOTALL | re.MULTILINE)
re_subdirs = re.compile(r'(.*?# @begin subdirs\s*?\n)(.*?)(# @end subdirs\s*?\n.*)', re.DOTALL | re.MULTILINE)


def update(args):
    shaders_root = args.rootdir / 'resources' / 'shader'
    subdir_map = {}

    for dir in sorted(shaders_root.glob('**/'), reverse=True):
        glsl_files = tuple(dir.glob('*.glsl'))

        try:
            subdirs = subdir_map[dir]
        except KeyError:
            subdirs = []

        if glsl_files:
            d = dir

            while d.resolve() != shaders_root.resolve():
                try:
                    subdir_map[d.parent].append(d)
                except KeyError:
                    subdir_map[d.parent] = [d]

                d = d.parent

        if glsl_files or subdirs:
            subdirs = tuple(sd.relative_to(dir) for sd in subdirs)
            glsl_files = tuple(f.relative_to(dir) for f in glsl_files)

            try:
                meson_text = (dir / 'meson.build').read_text()
            except FileNotFoundError:
                meson_text = meson_template

            glsl_filelist = '\n'.join(sorted("    '{!s}',".format(f) for f in glsl_files) + [''])
            meson_text = re_glsl.sub(r'\1{}\3'.format(glsl_filelist), meson_text)

            subdirs_list = '\n'.join(sorted("    '{!s}',".format(sd) for sd in subdirs) + [''])
            meson_text = re_subdirs.sub(r'\1{}\3'.format(subdirs_list), meson_text)

            update_text_file(dir / 'meson.build', meson_text)


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Update build defintions to include all GLSL files.', prog=args[0])
    add_common_args(parser)
    update(parser.parse_args(args[1:]))


if __name__ == '__main__':
    run_main(main)
