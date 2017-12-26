#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
    in_dir,
    meson_introspect, meson,
    add_common_args,
)

import subprocess


def main(args):
    with in_dir(args[1]):
        with open('saved_options.json', 'wb') as outfile:
            outfile.write(subprocess.check_output(['meson', 'introspect', '--buildoptions']))


if __name__ == '__main__':
    run_main(main)
