#!/usr/bin/env python3

from taiseilib.tempinstall import temp_install
from taiseilib.common import run_main

from pathlib import Path

import argparse
import subprocess
import shlex
import os
from sys import platform


def main(args):
    parser = argparse.ArgumentParser(description='Generate a .dmg package for macOS.', prog=args[0])

    parser.add_argument('output',
        help='The destination .dmg file',
    )

    parser.add_argument('build_dir',
        help='The build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    parser.add_argument('--universal',
        help='Universal build',
        default=False,
        action=argparse.BooleanOptionalAction,
    )

    parser.add_argument('combine_dir',
        help='The build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    args = parser.parse_args(args[1:])

    with temp_install(args.build_dir) as install_path:
        # for compiling on Darwin as 'genisoimage' doesn't exist
        # uses 'create-dmg' (brew install create-dmg)
        if platform == "darwin":
            command = shlex.split('''create-dmg
                --volname "Taisei"
                --volicon "Taisei.app/Contents/Resources/Taisei.icns"
                --window-pos 200 120
                --window-size 550 480
                --icon-size 64
                --icon "Taisei.app" 100 50
                --icon "README.txt" 50 200
                --icon "STORY.txt" 200 200
                --icon "GAME.html" 350 200
                --icon "COPYING" 125 350
                --icon "ENVIRON.html" 275 350
                --hide-extension "Taisei.app"
                --app-drop-link 300 50''')
            if args.universal:
                command += [args.output, str(args.combine_dir)]
            else:
                command += [args.output, str(install_path)]
        else:
            (install_path / 'Applications').symlink_to('/Applications')

            command = shlex.split('genisoimage -V Taisei -D -R -apple -no-pad -o') + [
                args.output,
                str(install_path),
            ]

        subprocess.check_call(command, cwd=str(install_path))

        print('\nPackage generated:', args.output)


if __name__ == '__main__':
    try:
        run_main(main)
    except subprocess.CalledProcessError as e:
        print("main dmg creation method failed, trying backup")
        import sys
        main(sys.argv)
