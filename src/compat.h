/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#ifndef __GNUC__ // clang defines this too
    #define __attribute__(...)
    #define __extension__
    #define PRAGMA(p)
#else
    #define PRAGMA(p) _Pragma(#p)
    #define USE_GNU_EXTENSIONS
#endif

#ifdef __USE_MINGW_ANSI_STDIO
    #define FORMAT_ATTR __MINGW_PRINTF_FORMAT
#else
    #define FORMAT_ATTR printf
#endif
