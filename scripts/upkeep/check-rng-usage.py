#!/usr/bin/env python3

import re
import ast
import json
import shlex
import subprocess
import sys
import code

from pathlib import Path

from taiseilib.common import (
    run_main,
    add_common_args,
)

from concurrent.futures import (
    ThreadPoolExecutor,
)


re_suspicious_rng = re.compile(r'rng_next\(\)[^;]+?rng_next\(\)', re.DOTALL)
re_fileinfo = re.compile(r'\n# +(\d+) +(".*?").*?\n')


def get_source_info(m):
    src = m.string[:m.end()]
    fileinfo_index = src.rindex('\n# ')
    src_since_anchor = src[fileinfo_index:]

    lineno, fname = re_fileinfo.search(src_since_anchor).groups()
    lineno = int(lineno)
    lineno += src_since_anchor.count('\n')
    fname = ast.literal_eval(fname)

    return (fname, lineno)


def expand_line(m):
    src = m.string
    start, end = m.span()

    while src[start] != '\n':
        start -= 1

    while src[end] != '\n':
        end += 1

    return src[start:m.start()] + '\x1b[1;31m' + src[m.start():m.end()] + '\x1b[0m' + src[m.end():end]


def find_suspicious_callsites(src):
    for m in re_suspicious_rng.finditer(src):
        fname, lineno = get_source_info(m)
        segment = expand_line(m)
        print(f'{fname}:{lineno}: Suspected RNG API misuse:  {segment}\n', file=sys.stderr)


def remove_flag(cmd, flag, nvalues=0):
    while True:
        try:
            i = cmd.index(flag)
            for n in range(1 + nvalues):
                del cmd[i]
        except ValueError:
            break


def preprocess(cmd_json):
    cmd = cmd_json['command']
    cmd = shlex.split(cmd)

    remove_flag(cmd, '-o')
    remove_flag(cmd, '-c')
    remove_flag(cmd, '-include', 1)
    remove_flag(cmd, '-fpch-preprocess')
    remove_flag(cmd, '-MD')
    remove_flag(cmd, '-MQ', 1)
    remove_flag(cmd, '-MF', 1)
    remove_flag(cmd, cmd_json['output'])

    cmd += ['-DRNG_API_CHECK', '-E', '-o', '-']

    # print('   '.join(cmd))
    p = subprocess.run(cmd, stdout=subprocess.PIPE, cwd=cmd_json['directory'])

    return p.stdout.decode('utf8')


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Check source code for suspicious RNG API usage.', prog=args[0])
    add_common_args(parser)

    args = parser.parse_args(args[1:])

    with (args.builddir / 'compile_commands.json').open() as f:
        compile_commands = json.load(f)

    with ThreadPoolExecutor() as ex:
        tuple(ex.map(lambda c: find_suspicious_callsites(preprocess(c)), compile_commands))


if __name__ == '__main__':
    run_main(main)
