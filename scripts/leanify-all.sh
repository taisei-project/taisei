#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
find resources atlas xdg -type f | parallel -j$(nproc) leanify
