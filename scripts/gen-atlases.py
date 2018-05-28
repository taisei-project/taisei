#!/usr/bin/env python3

from taiseilib.common import (
    ninja,
    run_main,
)

from pathlib import Path

import os, sys, pprint


def main(args):
    try:
        src_dir = Path(os.environ['MESON_SOURCE_ROOT'])
        build_dir = Path(os.environ['MESON_BUILD_ROOT'])
    except KeyError:
        print("This script should not be ran directly", file=sys.stderr)
        exit(1)

    ninja('-vC', build_dir, *args[1:])

if __name__ == '__main__':
    run_main(main)
