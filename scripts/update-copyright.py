#!/usr/bin/env python3

import pathlib, sys, re, itertools, contextlib

header = r"""\1/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */
\2\4\3
#include "taisei.h"

"""

header_regex = re.compile(r'^(#if 0.*?\s#endif\s*)?(?:\s*?/\*.*?\*/)?(?:\s*?(\n#pragma once))?(?:\s*?(\n// begin before-taisei-h\s*.*?\n.*?// end before-taisei-h\n?))?(?:\s*?#include "taisei.h")?(?:\s*?(\n#pragma once))?\s*', re.MULTILINE | re.DOTALL)

if __name__ == '__main__':
    for path in itertools.chain(*((pathlib.Path(__file__).parent.parent / 'src').glob(p) for p in ('**/*.[ch]', '**/*.[ch].in'))):
        with contextlib.suppress(IndexError):
            if path.suffixes[-2] == '.inc':
                continue

        path.write_text(header_regex.sub(header, path.read_text(), 1))
