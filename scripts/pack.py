#!/usr/bin/env python3

import re
import struct
import sys
import zlib
import dataclasses

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime

try:
    from compression.zstd import ZstdCompressor, ZstdDecompressor
    from zipfile import (
        compressor_names,
        ZIP_DEFLATED,
        ZIP_STORED,
        ZIP_ZSTANDARD,
        ZipFile,
        ZipInfo,
        ZSTANDARD_VERSION,
    )
except ImportError:
    from backports.zstd import ZstdCompressor, ZstdDecompressor
    from backports.zstd.zipfile import (
        compressor_names,
        ZIP_DEFLATED,
        ZIP_STORED,
        ZIP_ZSTANDARD,
        ZipFile,
        ZipInfo,
        ZSTANDARD_VERSION,
    )

from pathlib import Path, PurePosixPath

from taiseilib.common import (
    DirPathType,
    add_common_args,
    run_main,
    write_depfile,
)


ZIP64_LIMIT = 0xFFFFFFFF

# Seekable zstd format constants
# https://github.com/facebook/zstd/blob/dev/contrib/seekable_format/zstd_seekable_compression_format.md
SEEKABLE_SKIPPABLE_MAGIC  = 0x184D2A5E
SEEKABLE_FOOTER_MAGIC     = 0x8F92EAB1
SEEKABLE_FOOTER_SIZE      = 9   # Number_Of_Frames(4) + Seek_Table_Descriptor(1) + Seekable_Magic_Number(4)
SEEKABLE_ENTRY_SIZE       = 8   # Compressed_Size(4) + Decompressed_Size(4)


def check_zip64(path, uncompressed_size, compressed_size):
    if uncompressed_size > ZIP64_LIMIT or compressed_size > ZIP64_LIMIT:
        raise ValueError(
            f'{path}: file too large for zip32 '
            f'(uncompressed={uncompressed_size}, compressed={compressed_size}); '
            f'loader does not support zip64'
        )


def compress_zstd_seekable(data, level, frame_size):
    '''
    Compress data using the zstd seekable multi-frame format.
    The output is a valid zstd stream (backwards-compatible) followed by a
    skippable seek table frame, allowing random-access decompression.
    Each input chunk of `frame_size` bytes is compressed as an independent frame.
    '''
    cctx = ZstdCompressor(level=level)

    compressed_frames = []
    offset = 0

    while offset < len(data):
        chunk = data[offset:offset + frame_size]
        compressed_frames.append((cctx.compress(chunk, mode=ZstdCompressor.FLUSH_FRAME), len(chunk)))
        offset += frame_size

    # Seek_Table_Entries: Compressed_Size(4) + Decompressed_Size(4) per frame
    entries = b''.join(
        struct.pack('<II', len(cf), dc)
        for cf, dc in compressed_frames
    )

    # Seek_Table_Footer: Number_Of_Frames(4) + Seek_Table_Descriptor(1) + Seekable_Magic_Number(4)
    # Seek_Table_Descriptor = 0: Checksum_Flag=0, Reserved_Bits=0
    footer = struct.pack('<IBI', len(compressed_frames), 0, SEEKABLE_FOOTER_MAGIC)

    # Skippable frame: Skippable_Magic_Number(4) + Frame_Size(4) + frame content
    seek_table_content = entries + footer
    skippable_frame = struct.pack('<II', SEEKABLE_SKIPPABLE_MAGIC, len(seek_table_content)) + seek_table_content

    return b''.join(cf for cf, _ in compressed_frames) + skippable_frame


@dataclasses.dataclass
class PreparedEntry:
    path: Path
    arcname: str
    comp_type: int      # final storage type (after store/compress decision)
    data: bytes         # bytes to write verbatim
    file_size: int      # uncompressed size
    compress_size: int  # = len(data)
    crc: int


def _prepare_zst(zst_path, arcname):
    '''
    Worker: decompress zst to compute CRC and file size, read raw compressed
    bytes. Returns a PreparedEntry ready to be written to the zip.
    '''
    decompressor = ZstdDecompressor()

    with zst_path.open('rb') as zst_file:
        compress_size = zst_file.seek(0, 2)
        zst_file.seek(0, 0)
        data = zst_file.read()

        # Unfortunately we must decompress it to compute crc32.
        # We'll also compute file size from decompressed data instead of relying on frame headers.
        decomp_data = decompressor.decompress(data)
        file_size = len(decomp_data)
        crc = zlib.crc32(decomp_data)
        del decomp_data

        check_zip64(zst_path, file_size, compress_size)

    return PreparedEntry(
        path=zst_path,
        arcname=arcname,
        comp_type=ZIP_ZSTANDARD,
        data=data,
        file_size=file_size,
        compress_size=compress_size,
        crc=crc,
    )


def _prepare_file(path, arcname, comp_type, comp_level, zstd_frame_size, zstd_seekable_threshold, store_threshold):
    data = path.read_bytes()
    uncompressed_size = len(data)
    crc = zlib.crc32(data)

    if comp_type == ZIP_STORED:
        compressed_data = None
    elif comp_type == ZIP_ZSTANDARD:
        if uncompressed_size >= zstd_seekable_threshold:
            compressed_data = compress_zstd_seekable(data, comp_level, zstd_frame_size)
        else:
            cctx = ZstdCompressor(level=comp_level)
            compressed_data = cctx.compress(data, mode=ZstdCompressor.FLUSH_FRAME)
    elif comp_type == ZIP_DEFLATED:
        co = zlib.compressobj(comp_level, zlib.DEFLATED, -15)
        compressed_data = co.compress(data) + co.flush()
    else:
        raise ValueError(f'Unsupported compression type: {comp_type}')

    if compressed_data is not None and len(compressed_data) / max(uncompressed_size, 1) < store_threshold:
        store_type = comp_type
        store_data = compressed_data
    else:
        store_type = ZIP_STORED
        store_data = data

    compressed_size = len(store_data)
    check_zip64(path, uncompressed_size, compressed_size)

    return PreparedEntry(
        path=path,
        arcname=arcname,
        comp_type=store_type,
        data=store_data,
        file_size=uncompressed_size,
        compress_size=compressed_size,
        crc=crc,
    )


def _write_entry(zf, entry):
    log_entry(entry)

    zi = ZipInfo.from_file(entry.path, arcname=entry.arcname)
    zi.compress_type = entry.comp_type
    zi.file_size = entry.file_size
    zi.compress_size = entry.compress_size
    zi.CRC = entry.crc

    if entry.comp_type == ZIP_ZSTANDARD:
        zi.create_version = ZSTANDARD_VERSION
        zi.extract_version = ZSTANDARD_VERSION
        if not zi.external_attr:
            zi.external_attr = 0o600 << 16  # permissions: ?rw-------

    if zf._seekable:
        zf.fp.seek(zf.start_dir)

    zi.header_offset = zf.fp.tell()
    zf._writecheck(zi)
    zf._didModify = True
    zf.fp.write(zi.FileHeader(False))
    zf._writing = True

    try:
        zf.fp.write(entry.data)
        zf.filelist.append(zi)
        zf.NameToInfo[zi.filename] = zi
        zf.start_dir = zf.fp.tell()
    finally:
        zf._writing = False


def log_entry(entry):
    prefix = compressor_names.get(entry.comp_type, '???')

    ratio_str = ''
    if entry.comp_type != ZIP_STORED and entry.file_size > 0:
        ratio = entry.compress_size / entry.file_size
        ratio_str = f'{ratio:.1%}'

    print('%s% *s' % (ratio_str, 13 - len(ratio_str), prefix), '|', entry.arcname, '<--', str(entry.path))


def pack(args):
    nocompress_file = args.directory / '.nocompress'

    comp_type  = args.comp_type
    comp_level = args.comp_level
    if comp_level is None:
        comp_level = 20 if comp_type == ZIP_ZSTANDARD else 9

    try:
        nocompress = list(map(re.compile, filter(None, nocompress_file.read_text().strip().split('\n'))))
    except FileNotFoundError:
        nocompress = []
        nocompress_file = None

    zstd_frame_size = args.zstd_frame_size
    zstd_seekable_threshold = args.zstd_seekable_threshold
    if zstd_seekable_threshold is None:
        zstd_seekable_threshold = int(zstd_frame_size * 1.5)

    store_threshold = args.store_threshold

    dependencies = []

    prefix = ''
    for part in args.prefix.parts:
        prefix += part + '/'
    prefix = PurePosixPath(prefix)

    tasks = []

    for part in args.prefix.parts:
        name = str(PurePosixPath(*args.prefix.parts[:args.prefix.parts.index(part) + 1])) + '/'
        tasks.append(('dir', args.directory, name))

    for path in sorted(args.directory.glob('**/*')):
        if path.name[0] == '.' or any(path.match(x) for x in args.exclude):
            continue

        relpath = prefix / path.relative_to(args.directory)

        if path.is_dir():
            tasks.append(('dir', path, str(relpath) + '/'))
        else:
            dependencies.append(path)

            if path.suffix == '.zst':
                tasks.append(('zst', path, str(relpath.with_suffix(''))))
            else:
                ctype = comp_type

                for pattern in nocompress:
                    if pattern.match(str(relpath)):
                        ctype = ZIP_STORED
                        break

                tasks.append(('file', path, str(relpath), ctype))

    # Phase 1: submit all compression/CRC work in parallel, preserving order.
    with ThreadPoolExecutor() as executor:
        pending = []
        for task in tasks:
            if task[0] == 'dir':
                pending.append(task)
            elif task[0] == 'zst':
                _, path, arcname = task
                pending.append(('future', executor.submit(_prepare_zst, path, arcname)))
            else:
                _, path, arcname, ctype = task
                pending.append(('future', executor.submit(
                    _prepare_file, path, arcname, ctype, comp_level,
                    zstd_frame_size, zstd_seekable_threshold, store_threshold,
                )))

        # Phase 2: write results in order as futures complete.
        with ZipFile(args.output, 'w', ZIP_STORED) as zf:
            def add_directory_entry(name, realpath):
                zi = ZipInfo(name, datetime.fromtimestamp(realpath.stat().st_mtime).timetuple())
                zi.compress_type = ZIP_STORED
                zi.external_attr = 0o40755 << 16  # drwxr-xr-x
                print('          dir', '|', zi.filename, '<--', str(realpath))
                zf.writestr(zi, '')

            for item in pending:
                if item[0] == 'dir':
                    _, path, name = item
                    add_directory_entry(name, path)
                else:
                    _, future = item
                    _write_entry(zf, future.result())

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

    parser.add_argument('--prefix',
        type=PurePosixPath,
        default='',
        help='subdirectory within the archive to place everything into'
    )

    parser.set_defaults(comp_type=ZIP_ZSTANDARD)
    parser.add_argument('--zstd',
        dest='comp_type',
        action='store_const',
        const=ZIP_ZSTANDARD,
        help='compress with zstandard (default)'
    )
    parser.add_argument('--deflate',
        dest='comp_type',
        action='store_const',
        const=ZIP_DEFLATED,
        help='compress with deflate'
    )
    parser.add_argument('--store',
        dest='comp_type',
        action='store_const',
        const=ZIP_STORED,
        help='store without compression'
    )

    parser.add_argument('--level',
        dest='comp_level',
        type=int,
        default=None,
        metavar='LEVEL',
        help='compression level (default: 20 for zstd, 9 for deflate)'
    )

    parser.add_argument('--store-threshold',
        type=float,
        default=0.99,
        metavar='RATIO',
        help='store uncompressed if compressed size / original size >= this value (default: 0.99)'
    )

    parser.add_argument('--zstd-frame-size',
        type=int,
        default=16384,
        metavar='BYTES',
        help='uncompressed frame size for seekable zstd compression (default: 16384)'
    )

    parser.add_argument('--zstd-seekable-threshold',
        type=int,
        default=None,
        metavar='BYTES',
        help='minimum file size to use seekable zstd compression (default: frame size * 1.5)'
    )

    add_common_args(parser, depfile=True)

    args = parser.parse_args(args[1:])
    pack(args)


if __name__ == '__main__':
    run_main(main)
