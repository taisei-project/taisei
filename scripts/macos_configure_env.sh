#!/usr/bin/env sh

set -e

if [[ -z $1 || -z $2 ]]; then
	echo "Usage: macos_configure_env.sh /path/to/taisei/build /path/to/taisei\n"
	exit 0
fi

echo "export MAC_BUILD_DIR=$1" >> $(pwd)/.mac_env
echo "export TAISEI_ROOT=$2" >> $(pwd)/.mac_env

source $(pwd)/.mac_env

echo "export MESON_BUILD_ROOT_MACOS_X64=$MAC_BUILD_DIR/x64" >> $(pwd)/.mac_env
echo "export MESON_BUILD_ROOT_MACOS_AARCH64=$MAC_BUILD_DIR/aarch64" >> $(pwd)/.mac_env
echo "export MESON_BUILD_ROOT_MACOS_X64_COMPILED=$MAC_BUILD_DIR/compiled/x64" >> $(pwd)/.mac_env
echo "export MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED=$MAC_BUILD_DIR/compiled/aarch64" >> $(pwd)/.mac_env
echo "export MESON_BUILD_ROOT_MACOS_COMBINED=$MAC_BUILD_DIR/compiled/combined" >> $(pwd)/.mac_env
echo "export TAISEI_BIN_PATH=./Taisei.app/Contents/MacOS/Taisei" >> $(pwd)/.mac_env
