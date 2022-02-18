#!/usr/bin/env python3

from taiseilib.tempinstall import temp_install
from taiseilib.common import run_main

from pathlib import Path

import argparse
import subprocess
import shlex
import os
from sys import platform

def package_dmg(install_path, destination):
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
            --app-drop-link 300 50''') + [
                destination,
                str(install_path),
            ]
    else:
        (install_path / 'Applications').symlink_to('/Applications')
        command = shlex.split('genisoimage -V Taisei -D -R -apple -no-pad -o') + [
            destination,
            str(install_path),
        ]
    subprocess.check_call(command, cwd=str(install_path))

    print('\nPackage generated:', destination)

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

    parser.add_argument('installation',
        help='Specify a pre-built directory instead of making a new one from the build directory (i.e: in the case of a universal image)',
        type=Path,
        nargs='?',
    )

    args = parser.parse_args(args[1:])

    if args.installation is None:
        with temp_install(args.build_dir) as install_path:
            package_dmg(install_path, args.output)
    else:
        package_dmg(args.installation, args.output)



if __name__ == '__main__':
    run_main(main)
