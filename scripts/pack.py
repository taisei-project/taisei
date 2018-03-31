#!/usr/bin/env python3

import os
import sys
from zipfile import ZipFile, ZIP_DEFLATED

from taiseilib.common import write_depfile

assert(len(sys.argv) > 4)
archive = sys.argv[1]
sourcedir = sys.argv[2]
depfile = sys.argv[3]
directories = sys.argv[4:]

with ZipFile(archive, "w", ZIP_DEFLATED) as zf:
    for directory in directories:
        for root, dirs, files in os.walk(directory):
            for fn in files:
                if fn == 'meson.build':
                    continue

                abspath = os.path.join(root, fn)
                rel = os.path.join(os.path.basename(directory), os.path.relpath(abspath, directory))
                zf.write(abspath, rel)

    write_depfile(depfile, archive,
        [os.path.join(sourcedir, x) for x in zf.namelist()] + [__file__]
    )
