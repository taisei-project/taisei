#!/usr/bin/env bash

set -e

source "$(pwd)/.mac_env"

meson setup \
	--cross-file "$TAISEI_ROOT/misc/ci/common-options.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/forcefallback.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/macos-ugly-sdl3-hack.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/macos-x86_64-build-release.ini" \
	--prefix "$MESON_BUILD_ROOT_MACOS_X64_COMPILED" \
	"$MESON_BUILD_ROOT_MACOS_X64" \
	"$TAISEI_ROOT" "$@"

meson setup \
	--native-file "$TAISEI_ROOT/misc/ci/common-options.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/forcefallback.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/macos-ugly-sdl3-hack.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/macos-aarch64-build-release.ini" \
	--prefix "$MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED" \
	"$MESON_BUILD_ROOT_MACOS_AARCH64" \
	"$TAISEI_ROOT" "$@"
