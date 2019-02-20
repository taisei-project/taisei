#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
)

import subprocess
import pathlib
import sys
import re
import os


submodule_regex = re.compile('^.[0-9a-f]*? ([^ ]*)(?: .*?)?')


def handle_no_git():
    # TODO
    pass


def main(args):
    try:
        p = subprocess.run(['git', 'submodule', 'status'],
            cwd=os.environ.get('MESON_SOURCE_ROOT', pathlib.Path(__file__).parent),
            capture_output=True,
            universal_newlines=True
        )
    except FileNotFoundError:
        return handle_no_git()

    if p.returncode != 0:
        if p.stderr.startswith('fatal: not a git repository'):
            return handle_no_git()

        print('Git error:', p.stderr.strip(), file=sys.stderr)
        exit(1)

    for module in p.stdout.rstrip('\n').split('\n'):
        status = module[0]

        if status != ' ':
            match = submodule_regex.match(module)

            try:
                path = match[1]
            except TypeError:
                print('Failed to parse submodule: {0}'.format(module))
                continue

            if status == '+':
                print('Submodule `{0}` is not in sync with HEAD. Run `git submodule update` if this is not intended.'.format(path))
            elif status == '-':
                print('Submodule `{0}` is not initialized. Run `git submodule update --init` if this is not intended.'.format(path))
            elif status == 'U':
                print('Submodule `{0}` has unresolved conflicts.'.format(path))
            else:
                print('Status of submodule `{0}` is unknown: {1}'.format(path, module))


if __name__ == '__main__':
    run_main(main)
