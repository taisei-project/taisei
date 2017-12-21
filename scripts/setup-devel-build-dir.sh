#!/usr/bin/env bash

dst="$PWD"
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
src="$PWD"

exec ${MESON:-meson} \
    --buildtype=debug \
    --prefix="$dst/install" \
    -Dstrip=false \
    -Db_lto=false \
    -Db_ndebug=false \
    -Db_sanitize=address,undefined \
    "$src" "$dst" "$@"
