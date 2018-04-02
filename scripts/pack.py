#!/usr/bin/env python3

import os
import sys
from datetime import datetime
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED, ZIP_STORED

from taiseilib.common import write_depfile

assert(len(sys.argv) > 4)
archive = sys.argv[1]
sourcedir = sys.argv[2]
depfile = sys.argv[3]
directories = sys.argv[4:]

with ZipFile(archive, "w", ZIP_DEFLATED) as zf:
    handled_subdirs = set()

    for directory in directories:
        for root, dirs, files in os.walk(directory):
            for fn in files:
                abspath = os.path.join(root, fn)
                rel = os.path.join(os.path.basename(directory), os.path.relpath(abspath, directory))

                reldir, _, _ = rel.rpartition('/')

                if reldir not in handled_subdirs:
                    handled_subdirs.add(reldir)
                    zi = ZipInfo(reldir + "/", datetime.fromtimestamp(os.path.getmtime(directory)).timetuple())
                    zi.compress_type = ZIP_STORED
                    zi.external_attr = 0o40755 << 16 # drwxr-xr-x
                    zf.writestr(zi, "")

                zf.write(abspath, rel)

    write_depfile(depfile, archive,
        [os.path.join(sourcedir, x) for x in zf.namelist()] + [__file__]
    )
