#!/usr/bin/env bash

set -e

if [[ -z $1 || -z $2 ]]; then
	printf -- "Usage: macos_configure_env.sh /path/to/taisei/build /path/to/taisei\n\n"
	exit 0
fi

printf -- "export MAC_BUILD_DIR=%s\n" "$1" >> "$(pwd)/.mac_env"
printf -- "export TAISEI_ROOT=%s\n" "$2" >> "$(pwd)/.mac_env"

source $(pwd)/.mac_env

printf -- "export MESON_BUILD_ROOT_MACOS_X64=%s/x64\n" "$MAC_BUILD_DIR" >> "$(pwd)/.mac_env"
printf -- "export MESON_BUILD_ROOT_MACOS_AARCH64=%s/aarch64\n" "$MAC_BUILD_DIR" >> "$(pwd)/.mac_env"
printf -- "export MESON_BUILD_ROOT_MACOS_X64_COMPILED=%s/compiled/x64\n" "$MAC_BUILD_DIR" >> "$(pwd)/.mac_env"
printf -- "export MESON_BUILD_ROOT_MACOS_AARCH64_COMPILED=%s/compiled/aarch64\n" "$MAC_BUILD_DIR" >> "$(pwd)/.mac_env"
printf -- "export MESON_BUILD_ROOT_MACOS_COMBINED=%s/compiled/combined\n" "$MAC_BUILD_DIR" >> "$(pwd)/.mac_env"
printf -- "export TAISEI_BIN_PATH=./Taisei.app/Contents/MacOS/Taisei\n" >> "$(pwd)/.mac_env"
