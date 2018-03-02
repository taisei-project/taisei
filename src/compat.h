/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
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

#ifndef __has_attribute
	#ifdef __GNUC__
		#define __has_attribute(attr) 1
	#else
		#define __has_attribute(attr) 0
	#endif
#endif

#ifdef __USE_MINGW_ANSI_STDIO
	#define FORMAT_ATTR __MINGW_PRINTF_FORMAT
#else
	#define FORMAT_ATTR printf
#endif

#undef uint
typedef unsigned int uint;

#undef ushort
typedef unsigned short ushort;

#undef ulong
typedef unsigned long ulong;

#undef uchar
typedef unsigned char uchar;

#undef schar
typedef unsigned char schar;

/*
 * Abstract away the nasty GNU attribute syntax.
 */

// Function is a hot spot.
#define attr_hot \
	__attribute__ ((hot))

// Function has no side-effects.
#define attr_pure \
	__attribute__ ((pure))

// Function has no side-effects, return value depends on arguments only.
// Must not take pointer parameters, must not return void.
#define attr_const \
	__attribute__ ((const))

// Function never returns NULL.
#define attr_returns_nonnull \
	__attribute__ ((returns_nonnull))

// Function must be called with NULL as the last argument (for varargs functions).
#define attr_sentinel \
	__attribute__ ((sentinel))

// Identifier is meant to be possibly unused.
#define attr_unused \
	__attribute__ ((unused))

// Function or type is deprecated and should not be used.
#define attr_deprecated(msg) \
	__attribute__ ((deprecated(msg)))

// Function parameters at specified positions must not be NULL.
#define attr_nonnull(...) \
	__attribute__ ((nonnull(__VA_ARGS__)))

// The return value of this function must not be ignored.
#define attr_nodiscard \
	__attribute__ ((warn_unused_result))

// Function takes a printf-style format string and variadic arguments.
#define attr_printf(fmt_index, firstarg_index) \
	__attribute__ ((format(FORMAT_ATTR, fmt_index, firstarg_index)))

// Function must be inlined regardless of optimization settings.
#define attr_must_inline \
	__attribute__ ((always_inline))

// Structure must not be initialized with an implicit (non-designated) initializer.
#if __has_attribute(designated_init)
	#define attr_designated_init \
		__attribute__ ((designated_init))
#else
	#define attr_designated_init
#endif
