#!/usr/bin/env bash

IN="$1"
OUT="$2"

if [[ -z "$IN" ]] || [[ -z "$OUT" ]] || [[ ! -d "$IN" ]]; then
    >&2 echo "Usage: $0 <input_directory> <output.dmg>"
    exit 1
fi

ln -svf /Applications "$IN/Applications" || exit $?
genisoimage -V Taisei -D -R -apple -no-pad -o "$OUT" "$IN" || exit $?
rm -v "$IN/Applications"
