#!/usr/bin/python3

from taiseilib.common import (
    run_main,
    in_dir,
    meson_introspect,
    add_common_args,
)

from pathlib import Path

import argparse
import json
import shlex
import subprocess
import shutil
import re
import sys
import json


def main(args):
    parser = argparse.ArgumentParser(description='Regenerate a Meson build directory, attempting to preserve build options.', prog=args[0])

    parser.add_argument('build_dir',
        default=Path.cwd(),
        type=Path,
        nargs='?',
        help='the build directory (defaults to CWD)',
    )

    parser.add_argument('dest_build_dir',
        default=None,
        type=Path,
        nargs='?',
        help='the destination directory (defaults to same as build_dir)',
    )

    parser.add_argument('--meson',
        default=['meson'],
        type=shlex.split,
        help='override the Meson executable (useful for wrappers)',
    )

    parser.add_argument('meson_args',
        default=[],
        nargs='*',
        help='additional arguments for Meson',
    )

    add_common_args(parser)
    args = parser.parse_args(args[1:])

    if args.dest_build_dir is None:
        args.dest_build_dir = args.build_dir

    with in_dir(args.build_dir):
        try:
            build_options = meson_introspect('--buildoptions')
        except subprocess.SubprocessError:
            print("Warning: meson introspect failed, retrieving options from saved_options.json. This may not be up to date.", file=sys.stderr)

            with open('saved_options.json') as infile:
                build_options = json.loads(infile.read())

    regen_cmdline = args.meson + [
        str(args.rootdir.resolve(strict=True)),
        str(args.dest_build_dir.resolve(strict=False)),
    ]

    meson_options = set(re.findall(r'\[--([\w-]+)\s+.*?\]', subprocess.check_output(args.meson + ['--help']).decode('utf8'), re.A))

    def opt_str_value(value):
        if isinstance(value, bool):
            # Meson <= 0.43.0 bug
            return str(value).lower()
        else:
            return str(value)

    for opt in build_options:
        name = opt['name']
        value = opt_str_value(opt['value'])

        if name in meson_options:
            regen_cmdline.append('--{}={}'.format(name, value))
        else:
            regen_cmdline.append('-D{}={}'.format(name, value))

    regen_cmdline += args.meson_args

    args.dest_build_dir.mkdir(parents=True, exist_ok=True)

    with in_dir(args.dest_build_dir):
        obj = {
            'command': regen_cmdline,
            'build_options': build_options,
        }

        with Path('imported_options.json').open('w') as outfile:
            json.dump(obj, outfile,
                ensure_ascii=False,
                indent=4,
                sort_keys=True,
            )

        meson_dir = Path('meson-private')
        meson_dir_bak = meson_dir.with_name(meson_dir.name + '.bak')

        if meson_dir.is_dir():
            shutil.rmtree(meson_dir_bak, ignore_errors=True)
            meson_dir.rename(meson_dir_bak)

        subprocess.check_call(regen_cmdline)

        for opt in build_options:
            name = opt['name']
            value = opt_str_value(opt['value'])
            subprocess.call(args.meson + ['configure', '-D{}={}'.format(name, value)])

    print('')
    print("Regeneration done. This process is not 100% reliable; you may want to check the output of 'meson configure'")

if __name__ == '__main__':
    run_main(main)
