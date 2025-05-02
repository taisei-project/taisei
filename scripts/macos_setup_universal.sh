#!/usr/bin/env bash

set -e

source "$(pwd)/.mac_env"

mkdir -p "$MAC_BUILD_DIR/compiled" "$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib" "$MESON_BUILD_ROOT_MACOS_COMBINED"

# combine dylibs
lipo "$TAISEI_ROOT/angle-compiled/lib/macOS-arm64-dylib/libEGL.dylib" "$TAISEI_ROOT/angle-compiled/lib/macOS-x64-dylib/libEGL.dylib" -output "$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libEGL.dylib" -create
lipo "$TAISEI_ROOT/angle-compiled/lib/macOS-arm64-dylib/libGLESv2.dylib" "$TAISEI_ROOT/angle-compiled/lib/macOS-x64-dylib/libGLESv2.dylib" -output "$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libGLESv2.dylib" -create

meson setup \
	-Dangle_libegl=$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libEGL.dylib \
	-Dangle_libgles=$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libGLESv2.dylib \
	--cross-file "$TAISEI_ROOT/misc/ci/common-options.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/forcefallback.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/macos-ugly-sdl3-hack.ini" \
	--cross-file "$TAISEI_ROOT/misc/ci/macos-x86_64-build-release.ini" \
	--prefix "$MESON_BUILD_ROOT_MACOS_X64_COMPILED" \
	"$MESON_BUILD_ROOT_MACOS_X64" \
	"$TAISEI_ROOT" "$@"

meson setup \
	-Dangle_libegl=$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libEGL.dylib \
	-Dangle_libgles=$TAISEI_ROOT/angle-compiled/lib/macOS-universal-dylib/libGLESv2.dylib \
	--native-file "$TAISEI_ROOT/misc/ci/common-options.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/forcefallback.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/macos-ugly-sdl3-hack.ini" \
	--native-file "$TAISEI_ROOT/misc/ci/macos-aarch64-build-release.ini" \
	--prefix "$MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED" \
	"$MESON_BUILD_ROOT_MACOS_AARCH64" \
	"$TAISEI_ROOT" "$@"
