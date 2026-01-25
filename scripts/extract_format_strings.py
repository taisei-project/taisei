#!/usr/bin/env python3

import argparse
import re
from pathlib import Path
from taiseilib.common import (
    run_main,
    update_text_file,
)

def extract_strings(pot):
    msgid = re.compile(r'msgid (.*?)\nmsgstr', re.MULTILINE | re.DOTALL)
    matches = re.findall(msgid, pot)

    return matches

def format_outfile(strings):
    return '''\
#include "i18n/format_strings.h"

const char *format_string_list[] = {{
{}
}};

const int format_string_list_size = {};
'''.format(
    ',\n'.join(strings),
    len(strings),
)

def main(args):
    parser = argparse.ArgumentParser(description='Extract all original strings from a .pot file and write them as an array into a C source file.', prog=args[0])

    parser.add_argument('potfile',
        help='.pot file containing the strings',
        type=Path,
    )

    parser.add_argument('outfile',
        help='Destination .c file to save them to',
        type=Path,
    )

    args = parser.parse_args()

    with open(args.potfile, 'r') as f:
        strings = extract_strings(f.read())

    update_text_file(args.outfile, format_outfile(strings))

if __name__ == '__main__':
    run_main(main)
