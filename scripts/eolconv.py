#!/usr/bin/env python3

import sys

if __name__ == '__main__':
    try:
        linemode = sys.argv[1].lower()
        inpath = sys.argv[2]
        outpath = sys.argv[3]
        assert linemode in ('lf', 'crlf')
    except (IndexError, AssertionError):
        sys.stdout.write("Usage: %s [lf|crlf] infile outfile\n" % sys.argv[0])
        exit(1)

    bom = b'\xef\xbb\xbf'

    # Open in text mode to have Python normalize the line terminators for us
    with open(inpath, 'r', encoding='utf8') as infile:
        content = infile.read().encode('utf8')

    if linemode == 'crlf':
        content = content.replace(b'\n', b'\r\n')

        if not content.startswith(bom):
            content = bom + content
    else:
        if content.startswith(bom):
            content = content[len(bom):]

    with open(outpath, 'wb') as outfile:
        outfile.write(content)
