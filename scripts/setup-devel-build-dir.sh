#!/usr/bin/env bash

dst="$PWD"
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit $?
src="$PWD"

${MESON:-meson} \
    --buildtype=debug \
    --prefix="$dst/install" \
    -Dstrip=false \
    -Db_lto=false \
    -Db_ndebug=false \
    -Db_sanitize=address,undefined \
    -Dr_gles30=true \
    -Dshader_transpiler=true \
    -Duse_libcrypto=false \
    -Dobjpools=false \
    -Dpackage_data=false \
    -Dc_args=-march=native \
    "$@" "$dst" "$src"
