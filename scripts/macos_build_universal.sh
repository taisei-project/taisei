#!/usr/bin/env sh

set -e

if [[ -z $1 || -z $2 ]]; then
	echo "Usage: macos_build_universal.sh /path/to/taisei/build /path/to/taisei [--integrity-files]\n"
	exit 0
fi

export BUILD_DIR=$1
export TAISEI_ROOT=$2
export MESON_BUILD_ROOT_MACOS_X64=$BUILD_DIR/x64
export MESON_BUILD_ROOT_MACOS_AARCH64=$BUILD_DIR/aarch64
export MESON_BUILD_ROOT_MACOS_X64_COMPILED=$BUILD_DIR/compiled/x64
export MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED=$BUILD_DIR/compiled/aarch64
export MESON_BUILD_ROOT_MACOS_COMBINED=$BUILD_DIR/compiled/combined
export TAISEI_BIN_PATH=./Taisei.app/Contents/MacOS/Taisei

mkdir -p $BUILD_DIR/compiled $MESON_BUILD_ROOT_MACOS_COMBINED
meson setup --native-file $TAISEI_ROOT/misc/ci/macos-x86_64-build-release.ini --prefix $MESON_BUILD_ROOT_MACOS_X64_COMPILED $MESON_BUILD_ROOT_MACOS_X64 $TAISEI_ROOT
meson setup --cross-file $TAISEI_ROOT/misc/ci/macos-aarch64-build-release.ini --prefix $MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED $MESON_BUILD_ROOT_MACOS_AARCH64 $TAISEI_ROOT

meson install -C $MESON_BUILD_ROOT_MACOS_X64
meson install -C $MESON_BUILD_ROOT_MACOS_AARCH64

cp -a $MESON_BUILD_ROOT_MACOS_X64_COMPILED/ $MESON_BUILD_ROOT_MACOS_COMBINED
echo "Combining x64 and AArch64 binaries...\n"
lipo -create -output $MESON_BUILD_ROOT_MACOS_COMBINED/$TAISEI_BIN_PATH $MESON_BUILD_ROOT_MACOS_X64_COMPILED/$TAISEI_BIN_PATH $MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED/$TAISEI_BIN_PATH

TAISEI_VERSION=$($PWD/scripts/version.py)

$TAISEI_ROOT/scripts/macos-gen-dmg.py $BUILD_DIR/compiled/Taisei-$TAISEI_VERSION-universal.dmg $MESON_BUILD_ROOT_MACOS_X64 $MESON_BUILD_ROOT_MACOS_COMBINED $3
