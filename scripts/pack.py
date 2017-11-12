#!/usr/bin/env python3

import os
import sys
from zipfile import ZipFile, ZIP_DEFLATED

assert(len(sys.argv) > 4)
archive = sys.argv[1]
sourcedir = sys.argv[2]
depfile = sys.argv[3]
directories = sys.argv[4:]

with ZipFile(archive, "w", ZIP_DEFLATED) as zf:
    for directory in directories:
        for root, dirs, files in os.walk(directory):
            for fn in files:
                abspath = os.path.join(root, fn)
                rel = os.path.join(os.path.basename(directory), os.path.relpath(abspath, directory))
                zf.write(abspath, rel)

    with open(depfile, "w") as df:
        l = [archive + ":"] + [os.path.join(sourcedir, x) for x in zf.namelist()]
        df.write(" \\\n ".join(l))
