#!/usr/bin/env python3

import os
import sys
import pathlib
import taiseilib.version
import shutil


def main(args):
    def env_dir_path(v):
        try:
            s = os.environ[v]
        except KeyError:
            print("{} is not set".format(v))

        p = pathlib.Path(s)
        assert p.is_dir()
        return p

    src_root = env_dir_path('MESON_PROJECT_SOURCE_ROOT')
    dist_root = env_dir_path('MESON_PROJECT_DIST_ROOT')
    src_version = taiseilib.version.get(rootdir=src_root)

    (dist_root / taiseilib.version.OVERRIDE_FILE_NAME).write_text(src_version.string)

    remove_files = [
        '.dockerignore',
        '.gitattributes',
        '.gitignore',
        '.gitmodules',
        '.mailmap',
        'checkout',
        'pull',
    ]

    for fname in remove_files:
        (dist_root / fname).unlink(fname)

    shutil.rmtree(str(dist_root / '.github'))


if __name__ == '__main__':
    from taiseilib.common import run_main
    run_main(main)

