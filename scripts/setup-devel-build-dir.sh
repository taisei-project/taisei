#!/usr/bin/env bash

dst="$PWD"
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
src="$PWD"

${MESON:-meson} "$src" "$dst" \
    --buildtype=debug \
    --prefix="$dst/install"

${MESON:-meson} configure \
    -Dstrip=false \
    -Db_lto=false \
    -Db_ndebug=false \
    -Db_sanitize=address,undefined \
    "$@" "$dst"
