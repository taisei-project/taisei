#!/usr/bin/env bash

set -e

png="$1"

if ! [[ -f "$png"  ]]; then
    echo "Usage: $0 [file.png]" 1>&2
    exit 1
fi

msg() {
    echo " *** $@"
}

getsize() {
    local out="$(wc -c "$1")"
    echo -n "${out%% *}"
}

msg "leanifying..."
leanify -vvv "$png"
png_size="$(getsize "$png")"
id="$(identify "$png")"

if [[ "$id" == *" 16-bit "* ]]; then
    msg "16bpp image, WebP conversion skipped"
    exit 0
fi

msg "converting to WebP..."
webp="${png%%png}webp"
cwebp -progress -preset drawing -z 9 -lossless -q 100 "$png" -o "$webp"
webp_size="$(getsize "$webp")"

msg "PNG:  $png_size bytes"
msg "WebP: $webp_size bytes"

if [ "$png_size" -lt "$webp_size" ]; then
    msg "Keeping PNG ('$png'; $png_size bytes)"
    rm -fv "$webp"
else
    msg "Keeping WebP ('$webp'; $webp_size bytes)"
    rm -fv "$png"
fi
