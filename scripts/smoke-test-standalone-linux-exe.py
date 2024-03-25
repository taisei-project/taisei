#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
)

import argparse
import subprocess
import re
import sys

from pathlib import Path


allowed_patterns = set(re.compile(p) for p in (
    r'^/.*?/ld-[\d\w_-]+-[\d\w_-]+\.so\.\d+$',
    r'^lib(?:std)?c\+\+(?:abi)?\.so\.\d+$',
    r'^libc\.so(?:\.\d+)?$',
    r'^libdl\.so\.\d+$',
    r'^libgcc_s\.so\.\d+$',
    r'^libm\.so\.\d+$',
    r'^libpthread\.so\.\d+$',
    r'^linux-vdso\.so\.\d+$',
))


def main(args):
    parser = argparse.ArgumentParser(description='Complain if a Taisei executable dynamically links to any "suspicious" libraries', prog=args[0])

    parser.add_argument('taisei',
        help='The Taisei executable',
        type=Path,
    )

    args = parser.parse_args()

    p = subprocess.run(
        ['ldd', args.taisei],
        capture_output=True,
        check=True,
        text=True,
    )

    sys.stderr.write(p.stderr)

    libs = re.findall(r'^\s*([^\s]+)', p.stdout, re.M)
    sus = [l for l in libs if not any(re.match(p, l) for p in allowed_patterns)]

    if not sus:
        return

    sus.sort()
    print(
        args.taisei,
        'is dynamically linked to the following suspicious libraries:',
        *(f'\n\t{s}' for s in sus),
    file=sys.stderr)
    quit(42)


if __name__ == '__main__':
    run_main(main)


