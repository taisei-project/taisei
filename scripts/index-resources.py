#!/usr/bin/env python3

import hashlib

from pathlib import Path

from taiseilib.common import (
    DirPathType,
    add_common_args,
    run_main,
    write_depfile,
    update_text_file,
)


def quote(s):
    return '"{}"'.format(s.encode('unicode_escape').decode('latin-1').replace('"', '\\"'))


class DirEntry:
    def __init__(self, name):
        self.name = name
        self.subdirs = []
        self.subdirs_range = (0, 0)
        self.files = []
        self.files_range = (0, 0)


class FileEntry:
    def __init__(self, path):
        self.path = path
        self.name = path.name

    def unique_id(self):
        return hashlib.sha256(self.path.read_bytes()).hexdigest()


def make_range(begin, end):
    if begin == end:
        return 0, 0
    return begin, end - begin


def index(args):
    deps = [str(Path(__file__).resolve())]

    def excluded(path):
        return path.name[0] == '.' or any(path.match(x) for x in args.exclude)

    def make_union_map(dirs):
        u = {}

        for d in dirs:
            u.update(dict((p.relative_to(d), p) for p in d.glob('**/*') if not excluded(p)))

        return u

    pathmap = make_union_map(args.directories)
    import collections
    nestedmap_factory = lambda: collections.defaultdict(nestedmap_factory)
    nestedmap = nestedmap_factory()

    # Yes, it is probably stupid to build a nested data structure only to recursively scan it immediately, but it was the path of least resistance when I rewrote this crap last time and I don't care.

    for relpath, realpath in sorted(pathmap.items()):
        if not realpath.is_file():
            continue

        parts = relpath.parts
        target = nestedmap
        for part in relpath.parts[:-1]:
            target = target[part]
        target[relpath.name] = realpath

    rootent = DirEntry(None)

    def scan(node, name):
        dirent = DirEntry(name)

        for key, val in sorted(node.items()):
            if isinstance(val, dict):
                dirent.subdirs.append(scan(val, key))
            else:
                dirent.files.append(FileEntry(val))
                deps.append(val)

        return dirent

    rootent.subdirs.append(scan(nestedmap, None))

    lines = []
    out = lines.append

    ordered_dirs = []
    next_scan = [rootent]

    while next_scan:
        this_scan = tuple(next_scan)
        next_scan = []
        for dirent in this_scan:
            begin = len(ordered_dirs)
            for subdir in dirent.subdirs:
                ordered_dirs.append(subdir)
            end = len(ordered_dirs)
            dirent.subdirs_range = make_range(begin, end)
            next_scan += ordered_dirs[begin:end]

    findex = 0
    for n, dirent in enumerate(ordered_dirs):
        dirent.files_range = make_range(findex, findex + len(dirent.files))

        out("DIR({index:5}, {name}, {subdirs_ofs}, {subdirs_num}, {files_ofs}, {files_num})".format(
            index=n,
            name=(quote(dirent.name) if dirent.name is not None else "NULL"),
            subdirs_ofs=dirent.subdirs_range[0],
            subdirs_num=dirent.subdirs_range[1],
            files_ofs=dirent.files_range[0],
            files_num=dirent.files_range[1]))

        for fn, fentry in enumerate(dirent.files):
            out("FILE({index:4}, {unique_id}, {name}, {source_path})".format(
                index=findex + fn,
                name=quote(fentry.name),
                unique_id=quote(fentry.unique_id()),
                source_path=quote(str(fentry.path.resolve()))))

        findex += len(dirent.files)

    update_text_file(args.output, '\n'.join(lines))

    if args.depfile is not None:
        write_depfile(args.depfile, args.output, deps)


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Generate a static resource index with sha256 sums', prog=args[0])

    parser.add_argument('output',
        type=Path,
        help='the output index file path'
    )

    parser.add_argument('directories',
        metavar='directory',
        nargs='+',
        type=DirPathType,
        help='the directory to index'
    )

    parser.add_argument('--exclude',
        action='append',
        default=[],
        help='file exclusion pattern'
    )

    add_common_args(parser, depfile=True)

    args = parser.parse_args(args[1:])
    index(args)


if __name__ == '__main__':
    run_main(main)
