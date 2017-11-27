#!/usr/bin/env python3

import os
import sys
import version

assert(len(sys.argv) == 6)

root = sys.argv[1]
inpath = sys.argv[2]
outpath = sys.argv[3]
buildtype = sys.argv[4].strip()
v_fallback = sys.argv[5].strip()

version = version.get(fallback=v_fallback, rootdir=root)

if buildtype.lower().startswith("debug"):
    buildtype_def = "#define DEBUG_BUILD"
else:
    buildtype_def = "#define RELEASE_BUILD"

version = [("${TAISEI_VERSION_MAJOR}", version.major),
           ("${TAISEI_VERSION_MINOR}", version.minor),
           ("${TAISEI_VERSION_PATCH}", version.patch),
           ("${TAISEI_VERSION_TWEAK}", version.tweak),
           ("${TAISEI_VERSION}", version.string),
           ("${TAISEI_VERSION_FULL_STR}", version.full_string),
           ("${MESON_BUILD_TYPE}", buildtype),
           ("${ICONS_DIR}", os.path.join(root, "misc", "icons")),
           ("${BUILDTYPE_DEFINE}", buildtype_def)]

with open(inpath, "r") as infile:
    template = infile.read()

    for k, v in version:
        template = template.replace(k, str(v))

try:
    with open(outpath, "r+t") as outfile:
        contents = outfile.read()

        if contents == template:
            exit(0)

        outfile.seek(0)
        outfile.write(template)
        outfile.truncate()
except FileNotFoundError:
    with open(outpath, "w") as outfile:
        outfile.write(template)
