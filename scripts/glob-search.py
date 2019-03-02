#!/usr/bin/env python3

from taiseilib.common import (
    DirPathType,
    run_main,
)

from pathlib import Path
import fnmatch


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

    for path in (p.relative_to(args.directory) for p in args.directory.rglob('*')):
        path = str(path)
        for pattern in args.patterns:
            if fnmatch.fnmatchcase(path, pattern):
                print(path)
                break


if __name__ == '__main__':
    run_main(main)
