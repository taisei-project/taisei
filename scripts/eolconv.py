#!/usr/bin/env python3

import sys

if __name__ == '__main__':
    args = sys.argv[1:]
    linemode = args.pop(0).lower()
    use_bom = linemode == 'crlf'

    if args[0] == '--no-bom':
        use_bom = False
        args.pop(0)

    inpath = args.pop(0)
    outpath = args.pop(0)
    assert linemode in ('lf', 'crlf')

    bom = b'\xef\xbb\xbf'

    # Open in text mode to have Python normalize the line terminators for us
    with open(inpath, 'r', encoding='utf8') as infile:
        content = infile.read().encode('utf8')

    if linemode == 'crlf':
        content = content.replace(b'\n', b'\r\n')

    if use_bom and not content.startswith(bom):
        content = bom + content
    elif not use_bom and content.startswith(bom):
        content = content[len(bom):]

    with open(outpath, 'wb') as outfile:
        outfile.write(content)
