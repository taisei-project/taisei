#!/usr/bin/env python3

from taiseilib.tempinstall import temp_install
from taiseilib.common import run_main

import argparse
import subprocess
import shlex


def main(args):
    parser = argparse.ArgumentParser(description='Generate a macOS dmg package.', prog=args[0])

    parser.add_argument('output',
        help='The destination .dmg file',
    )

    parser.add_argument('build_dir',
        help='The build directory (defaults to CWD)',
        default='.',
        nargs='?',
    )

    args = parser.parse_args(args[1:])

    with temp_install(args.build_dir) as install_path:
        (install_path / 'Applications').symlink_to('/Applications')

        command = shlex.split('genisoimage -V Taisei -D -R -apple -no-pad -o') + [
            args.output,
            str(install_path),
        ]

        subprocess.check_call(command, cwd=str(install_path))

        print('\nPackage generated:', args.output)


if __name__ == '__main__':
    run_main(main)
