#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
)

from pathlib import Path, PureWindowsPath, PurePosixPath

import sys


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Because Windows is the worst.', prog=args[0])

    parser.add_argument('path',
        help='the path to operate on'
    )

    parser.add_argument('--from-windows',
        action='store_true',
        help='interpret source as a Windows path (default: POSIX path)'
    )

    parser.add_argument('--to-windows',
        action='store_true',
        help='output a Windows path (default: POSIX path)'
    )

    parser.add_argument('--escape-backslashes',
        action='store_true',
        help='escape any backslashes in the output'
    )

    args = parser.parse_args(args[1:])

    if args.from_windows:
        path = PureWindowsPath(args.path)
    else:
        path = PurePosixPath(args.path)

    if args.to_windows:
        out = str(PureWindowsPath(path))
    else:
        out = path.as_posix()

    if args.escape_backslashes:
        out = out.replace('\\', '\\\\')

    sys.stdout.write(out)


if __name__ == '__main__':
    run_main(main)
