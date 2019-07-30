#!/bin/bash
set -eo pipefail

source "$( dirname "${BASH_SOURCE[0]}" )/_env.sh"

MODE="release"
MODE_ARGS="-Ddebug=false"

if [[ "$1" == "debug" ]]; then
	MODE="debug"
	MODE_ARGS="-Ddebug=true --buildtype=debug -Db_lto=false -Denable_zip=false"
fi

mkdir -p "$BUILD_DIR/$MODE"

echo "Meson build $MODE"

"$SWITCH_DIR/crossfile.sh" > "$CROSSFILE"

meson --cross-file="$CROSSFILE" --default-library=static -Dstrip=false \
	--prefix="$OUT_DIR" --libdir=lib $MODE_ARGS \
	-Dr_gles30=true -Dr_default=gles30 -Dr_gl33=false \
	"$ROOT_DIR" "$BUILD_DIR/$MODE"

mkdir -p "$OUT_DIR"
ninja -C "$BUILD_DIR/$MODE" install

mv -vf "$OUT_DIR/taisei" "$OUT_DIR/taisei_$MODE.elf"

echo "Creating nacp: $OUT_DIR/taisei.nacp"
APP_VERSION="$(cd "$SWITCH_DIR" && git describe --tags --dirty)"
"$DEVKITPRO/tools/bin/nacptool" --create "$APP_TITLE" "$APP_AUTHOR" "$APP_VERSION" "$OUT_DIR/taisei.nacp"

echo "Creating nro: $OUT_DIR/taisei_$MODE.nro"
"$DEVKITPRO/tools/bin/elf2nro" "$OUT_DIR/taisei_$MODE.elf" "$OUT_DIR/taisei_$MODE.nro" \
	--nacp="$OUT_DIR/taisei.nacp" \
	--icon="$SWITCH_DIR/icon.jpg"
