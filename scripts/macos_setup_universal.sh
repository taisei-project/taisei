#!/usr/bin/env sh

set -e

source $(pwd)/.mac_env

mkdir -p $MAC_BUILD_DIR/compiled $MESON_BUILD_ROOT_MACOS_COMBINED
meson setup --native-file $TAISEI_ROOT/misc/ci/macos-x86_64-build-release.ini --prefix $MESON_BUILD_ROOT_MACOS_X64_COMPILED $MESON_BUILD_ROOT_MACOS_X64 $TAISEI_ROOT
meson setup --cross-file $TAISEI_ROOT/misc/ci/macos-aarch64-build-release.ini --prefix $MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED $MESON_BUILD_ROOT_MACOS_AARCH64 $TAISEI_ROOT
