#!/usr/bin/env python3

opts = '''MI_SECURE OFF
MI_DEBUG_FULL OFF
MI_PADDING OFF
MI_OVERRIDE ON
MI_XMALLOC OFF
MI_SHOW_ERRORS OFF
MI_TRACK_VALGRIND OFF
MI_TRACK_ASAN OFF
MI_TRACK_ETW OFF
MI_LOCAL_DYNAMIC_TLS OFF
MI_LIBC_MUSL OFF
MI_GUARDED OFF
MI_NO_PADDING OFF
MI_NO_THP OFF'''



for l in opts.split('\n'):
    o, v = l.split(' ')
    print(f"{repr(o)}: {v == 'ON'},")

opts = {
    'MI_SECURE': False,
    'MI_DEBUG_FULL': False,
    'MI_PADDING': False,
    'MI_OVERRIDE': True,
    'MI_XMALLOC': False,
    'MI_SHOW_ERRORS': False,
    'MI_TRACK_VALGRIND': False,
    'MI_TRACK_ASAN': False,
    'MI_TRACK_ETW': False,
    'MI_LOCAL_DYNAMIC_TLS': False,
    'MI_LIBC_MUSL': False,
    'MI_GUARDED': False,
    'MI_NO_PADDING': False,
    'MI_NO_THP': False,
}

text = '''

option(MI_SECURE            "Use full security mitigations (like guard pages, allocation randomization, double-free mitigation, and free-list corruption detection)" OFF)
option(MI_DEBUG_FULL        "Use full internal heap invariant checking in DEBUG mode (expensive)" OFF)
option(MI_PADDING           "Enable padding to detect heap block overflow (always on in DEBUG or SECURE mode, or with Valgrind/ASAN)" OFF)
option(MI_OVERRIDE          "Override the standard malloc interface (i.e. define entry points for 'malloc', 'free', etc)" ON)
option(MI_XMALLOC           "Enable abort() call on memory allocation failure by default" OFF)
option(MI_SHOW_ERRORS       "Show error and warning messages by default (only enabled by default in DEBUG mode)" OFF)
option(MI_TRACK_VALGRIND    "Compile with Valgrind support (adds a small overhead)" OFF)
option(MI_TRACK_ASAN        "Compile with address sanitizer support (adds a small overhead)" OFF)
option(MI_TRACK_ETW         "Compile with Windows event tracing (ETW) support (adds a small overhead)" OFF)
option(MI_USE_CXX           "Use the C++ compiler to compile the library (instead of the C compiler)" OFF)
option(MI_OPT_ARCH          "Only for optimized builds: turn on architecture specific optimizations (for arm64: '-march=armv8.1-a' (2016))" OFF)
option(MI_SEE_ASM           "Generate assembly files" OFF)
option(MI_OSX_INTERPOSE     "Use interpose to override standard malloc on macOS" ON)
option(MI_OSX_ZONE          "Use malloc zone to override standard malloc on macOS" ON)
option(MI_WIN_REDIRECT      "Use redirection module ('mimalloc-redirect') on Windows if compiling mimalloc as a DLL" ON)
option(MI_WIN_USE_FIXED_TLS "Use a fixed TLS slot on Windows to avoid extra tests in the malloc fast path" OFF)
option(MI_LOCAL_DYNAMIC_TLS "Use local-dynamic-tls, a slightly slower but dlopen-compatible thread local storage mechanism (Unix)" OFF)
option(MI_LIBC_MUSL         "Set this when linking with musl libc" OFF)
option(MI_BUILD_SHARED      "Build shared library" ON)
option(MI_BUILD_STATIC      "Build static library" ON)
option(MI_BUILD_OBJECT      "Build object library" ON)
option(MI_BUILD_TESTS       "Build test executables" ON)
option(MI_DEBUG_TSAN        "Build with thread sanitizer (needs clang)" OFF)
option(MI_DEBUG_UBSAN       "Build with undefined-behavior sanitizer (needs clang++)" OFF)
option(MI_GUARDED           "Build with guard pages behind certain object allocations (implies MI_NO_PADDING=ON)" OFF)
option(MI_SKIP_COLLECT_ON_EXIT "Skip collecting memory on program exit" OFF)
option(MI_NO_PADDING        "Force no use of padding even in DEBUG mode etc." OFF)
option(MI_INSTALL_TOPLEVEL  "Install directly into $CMAKE_INSTALL_PREFIX instead of PREFIX/lib/mimalloc-version" OFF)
option(MI_NO_THP            "Disable transparent huge pages support on Linux/Android for the mimalloc process only" OFF)
option(MI_EXTRA_CPPDEFS     "Extra pre-processor definitions (use as `-DMI_EXTRA_CPPDEFS=\"opt1=val1;opt2=val2\"`)" "")
'''

import re

fuck = re.compile(r'option\(([A-Z_]+)\s*(".*?")\s*(ON|OFF)\s*\)', re.MULTILINE)

for shit in fuck.findall(text):
    opt, desc, val = shit
    if opt in opts:
        desc = repr(eval(desc))
        val = val == 'ON'
        # print(f">> {opt} :: {desc} :: {val}")
        print(f"""
option(
    '{opt[3:].lower()}',
    type : 'boolean',
    value : {str(val).lower()},
    description : {desc}
)""")

for opt in opts:
    print(repr(opt[3:].lower()) + ',')
