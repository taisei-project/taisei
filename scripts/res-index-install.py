#!/usr/bin/env python3

import sys
import os
import shutil

from ast import literal_eval
from pathlib import Path

from taiseilib.common import (
    add_common_args,
    run_main,
)


def install(args):
    install_root = Path(os.environ['MESON_INSTALL_DESTDIR_PREFIX'])
    install_dir = install_root / args.subdir
    install_dir.mkdir(exist_ok=True, parents=True)
    installed_files = set(p.name for p in install_dir.iterdir())
    quiet = bool(os.environ.get('MESON_INSTALL_QUIET', False))

    for line in args.index.read_text().split('\n'):
        if not line.startswith('FILE('):
            continue

        _, unique_id, _, srcpath = literal_eval(line[4:])

        if unique_id in installed_files:
            continue

        dstpath = install_dir / unique_id
        assert not dstpath.exists()

        if not quiet:
            print('Installing resource {} as {}'.format(srcpath, dstpath))

        shutil.copy2(srcpath, dstpath)
        installed_files.add(unique_id)


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Install resources from res-index', prog=args[0])

    parser.add_argument('index',
        type=Path,
        help='the index file'
    )

    parser.add_argument('subdir',
        help='the target subdirectory (inside prefix)'
    )

    add_common_args(parser)

    args = parser.parse_args(args[1:])
    install(args)


if __name__ == '__main__':
    run_main(main)
