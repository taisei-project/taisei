#!/usr/bin/env sh

set -e

source $(pwd)/.mac_env

if [[ -z $1 ]]; then
	echo "Usage: macos_build_universal.sh VERSION [--integrity-files]\n"
	exit 0
fi

meson install -C $MESON_BUILD_ROOT_MACOS_X64
meson install -C $MESON_BUILD_ROOT_MACOS_AARCH64

cp -a $MESON_BUILD_ROOT_MACOS_X64_COMPILED/ $MESON_BUILD_ROOT_MACOS_COMBINED
echo "Combining x64 and AArch64 binaries...\n"
lipo -create -output $MESON_BUILD_ROOT_MACOS_COMBINED/$TAISEI_BIN_PATH $MESON_BUILD_ROOT_MACOS_X64_COMPILED/$TAISEI_BIN_PATH $MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED/$TAISEI_BIN_PATH

$TAISEI_ROOT/scripts/macos-gen-dmg.py $MAC_BUILD_DIR/compiled/Taisei-$1-universal.dmg $MESON_BUILD_ROOT_MACOS_X64 $MESON_BUILD_ROOT_MACOS_COMBINED $2
