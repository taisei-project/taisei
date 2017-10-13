#!/usr/bin/env python3

import sys

if __name__ == '__main__':
    try:
        inpath = sys.argv[1]
        outpath = sys.argv[2]
    except IndexError:
        sys.stdout.write("Usage: %s infile outfile\n")
        exit(1)

    # This would have been a one-liner with pathlib available...

    with open(inpath, 'rb') as infile:
        content = b'\xef\xbb\xbf' + infile.read().replace(b'\n', b'\r\n')

    with open(outpath, 'wb') as outfile:
        outfile.write(content)
