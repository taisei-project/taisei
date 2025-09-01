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
        ['fixup-source-files', 'check-rng-usage'],
        'update-glsl-sources',
        'gen-potfiles',
    )

    with ThreadPoolExecutor() as ex:
        def do_task(task):
            if isinstance(task, str):
                print('[upkeep] begin task', task)
                subprocess.check_call([scripts / f'{task}.py'] + args[1:])
                print('[upkeep] task', task, 'done')
            else:
                for t in task:
                    do_task(t)

        tuple(ex.map(do_task, tasks))


if __name__ == '__main__':
    run_main(main)
