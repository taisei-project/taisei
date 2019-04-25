#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
find resources -type f -name "*.png" | parallel -j$(nproc) scripts/optimize-img.sh
