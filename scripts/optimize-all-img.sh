#!/usr/bin/env bash

opwd="$PWD"
cd "$(dirname "${BASH_SOURCE[0]}")" || exit $?
script_dir="$PWD"
cd "$opwd" || exit $?

find "$@" -type f -name "*.png" -or -name '*.webp' | parallel "-j$(nproc)" "$script_dir/optimize-img.sh"
