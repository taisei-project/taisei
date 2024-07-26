#!/usr/bin/env bash

set -e

src="$1"

if ! [[ -f "$src" ]]; then
    printf -- "Usage: %s file.{png,webp}\n" "$0" >&2
    exit 1
fi

msg() {
    printf -- '%s: %s\n' "$src" "$@"
}

getsize() {
    local out="$(wc -c "$1")"
    printf -- '%s' "${out%% *}"
}

makewebp() {
    local input="$1"
    local output="$2"
    cwebp \
        -preset icon \
        -z 9 \
        -lossless \
        -q 100 \
        -m 6 \
        "$input" -o "$output" 2>&1 | while read line; do msg "[cwebp] $line"; done
}

fileid="$(file "$src")"
orig_size="$(getsize "$src")"

if [[ "$fileid" == *" Web/P image"* ]]; then
    if [[ "$fileid" == *" YUV color"* ]]; then
        msg "lossy WebP detected, will not process"
        exit
    fi

    msg "recompressing..."
    makewebp "$src" "$src~"
    new_size="$(getsize "$src~")"
    msg "old: $orig_size bytes"
    msg "new: $new_size bytes"

    if [[ "$new_size" -lt "$orig_size" ]]; then
        msg "keeping new version"
        mv "$src~" "$src"
    else
        msg "keeping original version"
        rm -f "$src~"
    fi
elif [[ "$fileid" == *" PNG image data"* ]]; then
    msg "leanifying..."
    leanify -vvv "$src" 2>&1 | while read line; do msg "[leanify] $line"; done
    png_size="$(getsize "$src")"
    id="$(identify "$src")"

    if [[ "$id" == *" 16-bit "* ]]; then
        msg "16bpp image, WebP conversion skipped"
    else
        msg "converting to WebP..."
        webp="${src%%png}webp"
        makewebp "$src" "$webp"
        webp_size="$(getsize "$webp")"
        msg "PNG:  $png_size bytes"
        msg "WebP: $webp_size bytes"

        if [[ "$png_size" -lt "$webp_size" ]]; then
            msg "keeping PNG ('$src'; $png_size bytes)"
            rm -f "$webp"
        else
            msg "keeping WebP ('$webp'; $webp_size bytes)"
            rm -f "$src"
        fi
    fi
else
    msg "unhandled file type" 1>&2
    exit 1
fi
