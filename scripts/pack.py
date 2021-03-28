#!/usr/bin/env python3

import os
import sys
import re
import zstandard
import zlib
import shutil

from datetime import (
    datetime,
)

import zipfile_zstd

from zipfile import (
    ZIP_DEFLATED,
    ZIP_STORED,
    ZIP_ZSTANDARD,
    ZSTANDARD_VERSION,
    ZipFile,
    ZipInfo,
    compressor_names,
)

from pathlib import Path

from taiseilib.common import (
    DirPathType,
    add_common_args,
    run_main,
    write_depfile,
)


zstd_decompressor = zstandard.ZstdDecompressor()


def write_zst_file(zf, zst_path, arcname):
    '''
    Add a file pre-compressed with zstd to the archive
    Of course zipfile doesn't support this use-case (because it sucks),
    so abuse generous access to its internals to implement it here.
    '''

    log_file(zst_path, arcname, ZIP_ZSTANDARD)

    zip64 = False

    with zst_path.open('rb') as zst_file:
        zst_size = zst_file.seek(0, 2)
        zst_file.seek(0, 0)

        zi = ZipInfo.from_file(str(zst_path), arcname=arcname)
        zi.compress_type = ZIP_ZSTANDARD
        zi.create_version = ZSTANDARD_VERSION
        zi.extract_version = ZSTANDARD_VERSION
        zi.compress_size = zst_size

        if not zi.external_attr:
            zi.external_attr = 0o600 << 16  # permissions: ?rw-------

        # Unfortunately we must decompress it to compute crc32.
        # We'll also compute file size from decompressed data instead of relying on frame headers.

        zi.file_size = 0
        zi.CRC = 0

        for chunk in zstd_decompressor.read_to_iter(zst_file):
            zi.file_size += len(chunk)
            zi.CRC = zlib.crc32(chunk, zi.CRC)

        if zf._seekable:
            zf.fp.seek(zf.start_dir)

        zi.header_offset = zf.fp.tell()

        zf._writecheck(zi)
        zf._didModify = True
        zf.fp.write(zi.FileHeader(zip64))
        zf._writing = True

        try:
            zst_file.seek(0, 0)
            shutil.copyfileobj(zst_file, zf.fp)
            assert zst_file.tell() == zi.compress_size

            zf.filelist.append(zi)
            zf.NameToInfo[zi.filename] = zi
            zf.start_dir = zf.fp.tell()
        finally:
            zf._writing = False


def log_file(path, arcname, comp_type=None):
    if str(arcname).endswith('/'):
        prefix = 'dir'
    else:
        prefix = compressor_names.get(comp_type, '???')

    print('% 12s' % prefix, '|', arcname, '<--', str(path))


def pack(args):
    nocompress_file = args.directory / '.nocompress'

    if 1:
        comp_type = ZIP_ZSTANDARD
        comp_level = 20
    else:
        comp_type = ZIP_DEFLATED
        comp_level = 9

    try:
        nocompress = list(map(re.compile, filter(None, nocompress_file.read_text().strip().split('\n'))))
    except FileNotFoundError:
        nocompress = []
        nocompress_file = None

    zkwargs = {}
    if (sys.version_info.major, sys.version_info.minor) >= (3, 7):
        zkwargs['compresslevel'] = comp_level

    dependencies = []

    with ZipFile(str(args.output), 'w', comp_type, **zkwargs) as zf:
        for path in sorted(args.directory.glob('**/*')):
            if path.name[0] == '.' or any(path.match(x) for x in args.exclude):
                continue

            relpath = path.relative_to(args.directory)

            if path.is_dir():
                zi = ZipInfo(str(relpath) + "/", datetime.fromtimestamp(path.stat().st_mtime).timetuple())
                zi.compress_type = ZIP_STORED
                zi.external_attr = 0o40755 << 16  # drwxr-xr-x
                log_file(path, zi.filename)
                zf.writestr(zi, '')
            else:
                dependencies.append(path)

                if path.suffix == '.zst':
                    write_zst_file(zf, path, str(relpath.with_suffix('')))
                else:
                    ctype = comp_type

                    for pattern in nocompress:
                        if pattern.match(str(relpath)):
                            ctype = ZIP_STORED
                            break

                    log_file(path, relpath, ctype)
                    zf.write(str(path), str(relpath), compress_type=ctype)

    if args.depfile is not None:
        if nocompress_file is not None:
            dependencies.append(nocompress_file)
        dependencies.append(Path(__file__).resolve())
        write_depfile(args.depfile, args.output, dependencies)


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
