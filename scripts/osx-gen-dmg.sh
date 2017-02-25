#!/bin/bash

IN="$1"
OUT="$2"

if [[ -z "$IN" ]] || [[ -z "$OUT" ]] || [[ ! -d "$IN" ]]; then
    >&2 echo "Usage: $0 <input_directory> <output.dmg>"
    exit 1
fi

ln -sv /Applications "$IN/Applications" || exit 1
genisoimage -V Taisei -D -R -apple -no-pad -o "$OUT" "$IN" || exit $?
rm -v "$IN/Applications"
