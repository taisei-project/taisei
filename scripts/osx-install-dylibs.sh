#!/usr/bin/env bash

if [[ "$1" != "--iknowwhatimdoing" ]]; then
    >&2 echo "This script should not be ran directly."
    exit 1
fi

if [[ -z "$BASH_VERSINFO" ]] || [[ "$BASH_VERSINFO" -lt 4 ]]; then
    >&2 echo "Your Bash version is too old. 4.0+ is required."
    exit 1
fi

shift

EXE_PATH="$1"
DYLIB_PATH="$2"
DYLIB_REL_PATH="$3"
OSX_ROOT="$4"
OSX_CMD_PREFIX="$5"

mkdir -p "$DYLIB_PATH" || exit 2
[[ -f "$EXE_PATH" ]] || exit 3
[[ -d "$OSX_ROOT" ]] || exit 4

export PATH="$OSX_ROOT/bin:$PATH"

otool="${OSX_CMD_PREFIX}otool"
install_name_tool="${OSX_CMD_PREFIX}install_name_tool"

$otool --version

rm -vf "$DYLIB_PATH"/*.dylib

declare -A handled_libs

function handle_dylibs {
    for lib in $($otool -L "$1" | sed -e '/:$/d' -e 's/\s*//' -e 's/\s.*//' -e '/libSystem/d' -e '/.*\.dylib/!d' -e '/^\/usr\//d' | sort | uniq); do
        libpath="$OSX_ROOT$lib"
        libname="${lib##*/}"

        $install_name_tool "$1" -change "$lib" "@executable_path/$DYLIB_REL_PATH/$libname" || exit 5

        if [[ -n "${handled_libs[$libname]}" ]]; then
            continue
        fi

        cp -v "$libpath" "$DYLIB_PATH/$libname" || exit 6

        handled_libs[$libname]=1
        handle_dylibs "$DYLIB_PATH/$libname"
    done

    $otool -L "$1"
}

handle_dylibs "$EXE_PATH"
