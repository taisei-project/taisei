#!/usr/bin/env python3

from taiseilib.tempinstall import (
    temp_install,
)

from taiseilib.common import (
    add_common_args,
    run_main,
    TaiseiError,
)

from pathlib import (
    Path,
)

from zipfile import (
    ZipFile,
    ZIP_DEFLATED,
)

import argparse
import subprocess
import shlex
import os


def main(args):
    parser = argparse.ArgumentParser(description='Generate an ZIP distribution.', prog=args[0])

    parser.add_argument('build_dir',
        help='The build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    parser.add_argument('output',
        help='The output file path',
        type=Path,
    )

    add_common_args(parser)
    args = parser.parse_args(args[1:])

    with temp_install(args.build_dir) as install_dir, ZipFile(str(args.output), "w", ZIP_DEFLATED) as zfile:
        for path in install_dir.glob('**/*'):
            if not path.is_dir():
                rel = str(path.relative_to(install_dir))
                print("Adding file {}".format(rel))
                zfile.write(str(path), rel)

    print("Generated package {}".format(str(args.output)))

if __name__ == '__main__':
    run_main(main)
