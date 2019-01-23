#!/usr/bin/env python3

from taiseilib.common import (
    ninja,
    run_main,
    add_common_args,
)

from concurrent.futures import (
    ThreadPoolExecutor,
)

from pathlib import Path

import os
import sys
import subprocess


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Perform all automated routine maintenance tasks.', prog=args[0])
    add_common_args(parser)
    pargs = parser.parse_args(args[1:])

    scripts = pargs.rootdir / 'scripts' / 'upkeep'

    tasks = (
        'fixup-source-files',
        'update-glsl-sources',
    )

    with ThreadPoolExecutor() as ex:
        tuple(ex.map(lambda task: subprocess.check_call([scripts / f'{task}.py'] + args[1:]), tasks))


if __name__ == '__main__':
    run_main(main)
