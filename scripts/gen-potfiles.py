#!/usr/bin/env python3

import argparse
import glob

parser = argparse.ArgumentParser(description='Generate the POTFILES file for gettext')

parser.add_argument('-o', '--output', required=True, help='destination path for POTFILES')
args = parser.parse_args()

with open(args.output, 'w') as f:
    f.write("\n".join(glob.iglob("src/**/*.c",recursive=True)))
    f.write("\n")
    f.write("\n".join(glob.iglob("src/**/*.h",recursive=True)))

