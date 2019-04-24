#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
    update_text_file,
)

import argparse
import subprocess

from pathlib import Path


THE_STUPID_FUCKING_PLACEHOLDER = '[[THIS IS AN INTENTIONAL WHITESPACE, DO NOT "OPTIMIZE" IT OUT YOU ABSOLUTE FUCKING MORON]]'


def main(args):
    parser = argparse.ArgumentParser(description='Run emcc for the final code generation stage, then replace stupid fucking placeholders in the HTML output with actual fucking whitespaces that would have been fucked by Emscripten\'s fucked-in-the-head HTML minifier if they were encoded as the god-fucking-damned regular whitespaces that they are fucking supposed to be in the first place.', prog=args[0])

    parser.add_argument('output',
        help='the html output, in which the stupid fucking placeholders shall be replaced',
        type=Path,
    )

    parser.add_argument('compiler',
        nargs='*',
        help='the emcc command with arguments'
    )

    args = parser.parse_args(args[1:])

    print(args.compiler + ['-o', str(args.output)])
    proc = subprocess.run(args.compiler + ['-o', str(args.output)])

    if proc.returncode != 0:
        exit(proc.returncode)

    update_text_file(args.output, args.output.read_text().replace(THE_STUPID_FUCKING_PLACEHOLDER, ' '))


if __name__ == '__main__':
    run_main(main)
