#!/usr/bin/env python3

import sys
import os
import shutil

from pathlib import Path

from taiseilib.common import (
    add_common_args,
    run_main,
)

def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Install resources from res-index', prog=args[0])

    parser.add_argument('remove_items',
        type=Path,
        help='the files and/or directories to remove (relative to install destdir + prefix)',
        nargs='*',
    )

    add_common_args(parser)

    args = parser.parse_args(args[1:])

    install_root = Path(os.environ['MESON_INSTALL_DESTDIR_PREFIX'])
    quiet = bool(os.environ.get('MESON_INSTALL_QUIET', False))
    dryrun = bool(os.environ.get('MESON_INSTALL_DRY_RUN', False))

    def log(*s):
        if not quiet:
            print(*s)

    for item in args.remove_items:
        itempath = (install_root / item)

        if dryrun:
            log(f'Would remove {itempath}')
        elif itempath.exists():
            if itempath.is_dir():
                log(f'Removing directory {itempath}')
                itempath.rmdir()
            else:
                log(f'Removing file {itempath}')
                itempath.unlink()
        else:
            log(f'Skipping non-existent file {itempath}')


if __name__ == '__main__':
    run_main(main)
