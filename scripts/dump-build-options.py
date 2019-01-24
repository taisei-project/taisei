#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
    in_dir,
)

import os
import subprocess


def main(args):
    with in_dir(os.environ['MESON_BUILD_ROOT']):
        with open('saved_options.json', 'wb') as outfile:
            outfile.write(subprocess.check_output(['meson', 'introspect', '--buildoptions']))


if __name__ == '__main__':
    run_main(main)
