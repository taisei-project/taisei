#!/usr/bin/env bash

png="$1"

if ! [[ -f "$png"  ]]; then
    echo "Usage: $0 [file.png]" 1>&2
    exit 1
fi

id="$(identify "$png")"

if [[ $? != 0 ]]; then
    exit $?
elif [[ "$id" == *" 16-bit "* ]]; then
    echo "Won't convert 16-bit image, leanifying instead" 1>&2
    exec leanify -v "$png"
else
    cwebp -progress -preset drawing -z 9 -lossless -q 100 "$png" -o "${png%%png}webp" || exit $?
    rm -v "$png"
fi
