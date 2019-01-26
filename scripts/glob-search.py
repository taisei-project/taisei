#!/usr/bin/env python3

from taiseilib.common import (
    DirPathType,
    run_main,
)

from pathlib import Path


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Search directory by multiple glob patterns.', prog=args[0])

    parser.add_argument('directory',
        type=DirPathType,
        help='the directory to search'
    )

    parser.add_argument('patterns',
        metavar='pattern',
        nargs='+',
        help='a glob pattern'
    )

    args = parser.parse_args(args[1:])

    for pattern in args.patterns:
        for path in args.directory.glob(pattern):
            print(path.relative_to(args.directory))


if __name__ == '__main__':
    run_main(main)
