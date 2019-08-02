#!/usr/bin/env python3

import os
import sys
import re

from datetime import (
    datetime,
)

from zipfile import (
    ZipFile,
    ZipInfo,
    ZIP_DEFLATED,
    ZIP_STORED,
)

from pathlib import Path

from taiseilib.common import (
    DirPathType,
    add_common_args,
    run_main,
    write_depfile,
)

def pack(args):
    nocompress_file = args.directory / '.nocompress'

    try:
        nocompress = list(map(re.compile, filter(None, nocompress_file.read_text().strip().split('\n'))))
    except FileNotFoundError:
        nocompress = []
        nocompress_file = None

    zkwargs = {}
    if (sys.version_info.major, sys.version_info.minor) >= (3, 7):
        zkwargs['compresslevel'] = 9

    with ZipFile(str(args.output), 'w', ZIP_DEFLATED, **zkwargs) as zf:
        for path in sorted(args.directory.glob('**/*')):
            if path.name[0] == '.' or any(path.match(x) for x in args.exclude):
                continue

            relpath = path.relative_to(args.directory)

            if path.is_dir():
                zi = ZipInfo(str(relpath) + "/", datetime.fromtimestamp(path.stat().st_mtime).timetuple())
                zi.compress_type = ZIP_STORED
                zi.external_attr = 0o40755 << 16  # drwxr-xr-x
                zf.writestr(zi, '')
            else:
                ctype = ZIP_DEFLATED

                for pattern in nocompress:
                    if pattern.match(str(relpath)):
                        ctype = ZIP_STORED
                        break

                zf.write(str(path), str(relpath), compress_type=ctype)

        if args.depfile is not None:
            write_depfile(args.depfile, args.output,
                [args.directory.resolve() / x for x in zf.namelist()] +
                [str(Path(__file__).resolve())] +
                list(filter(None, [nocompress_file]))
            )


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Package game assets.', prog=args[0])

    parser.add_argument('directory',
        type=DirPathType,
        help='the source package directory'
    )

    parser.add_argument('output',
        type=Path,
        help='the output archive path'
    )

    parser.add_argument('--exclude',
        action='append',
        default=[],
        help='file exclusion pattern'
    )

    add_common_args(parser, depfile=True)

    args = parser.parse_args(args[1:])
    pack(args)


if __name__ == '__main__':
    run_main(main)
