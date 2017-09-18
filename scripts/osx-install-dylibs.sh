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

EXE_PATH="$1"; shift
DYLIB_PATH="$1"; shift
DYLIB_INTERMEDIATE_PATH="$1"; shift
DYLIB_REL_PATH="$1"; shift
OSX_LIB_PATH="$1"; shift
OSX_TOOL_PATH="$1"; shift
OSX_TOOL_PREFIX="$1"; shift
GENERATED_SCRIPT_PATH="$1"; shift

mkdir -p "$DYLIB_PATH" || exit 2
mkdir -p "$DYLIB_INTERMEDIATE_PATH" || exit 3

[[ -f "$EXE_PATH" ]] || exit 4

if [[ -n "$OSX_TOOL_PATH" ]]; then
    export PATH="$OSX_TOOL_PATH:$PATH"
fi

otool="${OSX_TOOL_PREFIX}otool"
install_name_tool="${OSX_TOOL_PREFIX}install_name_tool"

$otool --version

rm -vf "$DYLIB_INTERMEDIATE_PATH"/*.dylib "$DYLIB_PATH"/*.dylib

declare -A handled_libs

function find_dylib {
    for p in $(echo "$OSX_LIB_PATH" | tr ':' '\n'); do
        if [[ -f "$p/$1" ]]; then
            echo "$p/$1"
            return 0
        fi
    done

    return 1
}

function handle_dylibs {
    $otool -L "$1"

    for lib in $($otool -L "$1" | sed -e '/:$/d' -e 's/[[:blank:]]*//' -e 's/[[:blank:]].*//' -e '/libSystem/d' -e '/.*\.dylib/!d' -e '/^\/usr\//d' | sort | uniq); do
        libname="${lib##*/}"
        libpath="$(find_dylib "$libname")"

        if [[ -z "$libpath" ]]; then
            >&2 "Library $libname not found in OSX_LIB_PATH (required by $1)"
        fi

        $install_name_tool "$1" -change "$lib" "@executable_path/$DYLIB_REL_PATH/$libname" || exit 5

        if [[ -n "${handled_libs[$libname]}" ]]; then
            continue
        fi

        cp -v "$libpath" "$DYLIB_INTERMEDIATE_PATH/$libname" || exit 6

        handled_libs[$libname]=1
        handle_dylibs "$DYLIB_INTERMEDIATE_PATH/$libname"
    done
}

handle_dylibs "$EXE_PATH"
