#!/usr/bin/env python3

from taiseilib.common import (
    add_common_args,
    run_main,
    update_text_file,
    TaiseiError,
)

from concurrent.futures import (
    ThreadPoolExecutor,
)

import sys
import re
import itertools
import contextlib

from pathlib import Path

# The maintenance script is actually a pile of unmaintainable regular expressions!
# What a shocking twist!

copyright_comment_prefix = r"""/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---"""

copyright_comment_default_copyrights = r"""
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
"""

use_pragma_once = True

guard_prefix = 'IGUARD_'
guard_chars = 'A-Za-z0-9_'

guard_pattern = rf'{guard_prefix}[{guard_chars}]+'
comment_pattern = rf'(?:[ \t]*?//[^\n]*)?'
hash_pattern = rf'\s*?#[ \t]*'
pragma_once_pattern = rf'{hash_pattern}pragma[ \t]+once{comment_pattern}'
guard_header_pattern = rf'{hash_pattern}ifndef[ \t]+{guard_pattern}{comment_pattern}\s*?\n{hash_pattern}define\s+{guard_pattern}{comment_pattern}'
pragma_once_or_guard_header_pattern = rf'(?:\s*?(\n{pragma_once_pattern}|\n{guard_header_pattern}))?'
pre_taiseih_block_pattern = rf'// BEGIN before-taisei-h\s*.*?\n.*?// END before-taisei-h\n'
include_taiseih_pattern = rf'{hash_pattern}include[ \t]+"taisei.h"'

pragma_regex = re.compile(rf'{pragma_once_pattern}\s*?\n+(.*)\n+$', re.DOTALL)
guard_regex = re.compile(rf'\s*?\n{guard_header_pattern}\s*?\n+(.*?)\n{hash_pattern}endif{comment_pattern}\s*$', re.DOTALL)
header_regex = re.compile(rf'^(?:(?:\s*?/\*.*?---)(.*?)\*/)?{pragma_once_or_guard_header_pattern}(?:\s*?(\n{pre_taiseih_block_pattern}))?{pragma_once_or_guard_header_pattern}(?:\s*?{include_taiseih_pattern})?\s*', re.DOTALL)
badchar_regex = re.compile(rf'[^{guard_chars}]')
missing_guard_test_regex = re.compile(rf'^(?:\s*?/\*.*?\*/)\n(?:\n{pre_taiseih_block_pattern})?\n{include_taiseih_pattern}\n', re.DOTALL)


def header_template(match):
    copyright_contributors = match.group(1) or ''
    guard1 = match.group(2) or ''
    pre_taisei_h = match.group(3) or ''
    guard2 = match.group(4) or ''

    if not copyright_contributors.strip():
        copyright_contributors = copyright_comment_default_copyrights

    return (
        f'{copyright_comment_prefix}' +
        (copyright_contributors + '*/\n' + guard1 + guard2 + pre_taisei_h) +
        '\n#include "taisei.h"\n\n'
    )


class Janitor:
    def __init__(self, root):
        self.root = root
        self.used_guards = set()

    def get_guard_name(self, path):
        relpath = path.relative_to(self.root)
        return f'{guard_prefix}{badchar_regex.sub("_", str(relpath))}'

    def get_guard_template(self, guard, path):
        # Workaround stupid gcc warning
        is_pch = str(path).endswith('/src/pch/taisei_pch.h')

        if use_pragma_once and not is_pch:
            return (
                r'\n\n'
                rf'#pragma once\n'
                r'\1\n'
            )
        else:
            return (
                r'\n\n'
                rf'#ifndef {guard}\n'
                rf'#define {guard}\n\n'
                r'\1\n\n'
                rf'#endif // {guard}\n'
            )

    def transform_include_guards(self, text, path):
        guard = self.get_guard_name(path)
        template = self.get_guard_template(guard, path)

        # replace #pragma once
        text = pragma_regex.sub(template, text, 1)

        # replace outdated guards, if any
        text = guard_regex.sub(template, text, 1)

        return text

    def update_header(self, text):
        text = header_regex.sub(header_template, text, 1)
        return text

    def do_maintenance(self, path):
        guard = self.get_guard_name(path)

        if guard in self.used_guards:
            raise TaiseiError('Guard name conflict ({guard}), please fix the naming scheme')

        self.used_guards.add(guard)

        text = path.read_text()
        text = self.update_header(text)
        text = self.transform_include_guards(text, path)

        if path.suffixes[-1] == '.h':
            if missing_guard_test_regex.match(text):
                print(f"W: {path}: header missing include guard, fixing!", file=sys.stderr)
                text = text.replace('#include "taisei.h"', '#pragma once\n#include "taisei.h"')
                text = self.update_header(text)
                text = self.transform_include_guards(text, path)

        update_text_file(path, text)

    def iter_files(self):
        for path in itertools.chain(*(self.root.glob(p) for p in ('**/*.[ch]', '**/*.[ch].in'))):
            with contextlib.suppress(IndexError):
                if path.suffixes[-2] == '.inc':
                    continue

            yield path


def main(args):
    import argparse
    parser = argparse.ArgumentParser(description='Fix-up all C sources and headers - add/update copyright header, include guards, etc.', prog=args[0])
    add_common_args(parser)
    pargs = parser.parse_args(args[1:])

    j = Janitor(pargs.rootdir / 'src')

    with ThreadPoolExecutor() as ex:
        tuple(ex.map(j.do_maintenance, j.iter_files()))


if __name__ == '__main__':
    run_main(main)
