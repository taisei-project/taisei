#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
find resources xdg misc -type f | parallel -j$(nproc) leanify
