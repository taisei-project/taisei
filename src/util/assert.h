/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/macrohax.h"

#ifndef TAISEI_BUILDCONF_HAVE_STATIC_ASSERT_WITHOUT_MSG
	#undef static_assert
	#define _static_assert_0(cond) _Static_assert(cond, #cond)
	#define _static_assert_1(cond, msg) _Static_assert(cond, msg)
	#define static_assert(cond, ...) \
		MACROHAX_OVERLOAD_NARGS(_static_assert_, __VA_ARGS__)(cond, ##__VA_ARGS__)
#endif

void _ts_assert_fail(
	const char *cond,
	const char *msg,
	const char *func,
	const char *file,
	int line,
	bool use_log
);

#undef assert

#if defined(__EMSCRIPTEN__)
	void _emscripten_trap(void);
	#define TRAP() _emscripten_trap()
#elif defined(NDEBUG)
	#define TRAP() abort()
#elif defined(__clang__)
	#define TRAP() __builtin_debugtrap()
#elif defined(__GNUC__)
	#define TRAP() __builtin_trap()
#elif TAISEI_BUILDCONF_HAVE_POSIX
	#include <signal.h>
	#define TRAP() raise(SIGTRAP)
#else
	#define TRAP() abort()
#endif

#ifdef NDEBUG
    #define _assert(cond, msg, uselog)
	#define _assume(cond, msg, uselog) ASSUME(cond)
#else
    #define _assert(cond, msg, uselog) ({ \
		if(UNLIKELY(!(cond))) { \
			_ts_assert_fail(#cond, msg, __func__, _TAISEI_SRC_FILE, __LINE__, uselog); \
			TRAP(); \
			__builtin_unreachable(); \
		} \
	})
	#define _assume(cond, msg, uselog) _assert(cond, msg, uselog)
#endif

#define _assert_0(cond)      _assert(cond, NULL, true)
#define _assert_1(cond, msg) _assert(cond, msg, true)
#define assert(cond, ...) \
	MACROHAX_OVERLOAD_NARGS(_assert_, __VA_ARGS__)(cond, ##__VA_ARGS__)

#define _assert_nolog_0(cond)      _assert(cond, NULL, false)
#define _assert_nolog_1(cond, msg) _assert(cond, msg, false)
#define assert_nolog(cond, ...) \
	MACROHAX_OVERLOAD_NARGS(_assert_nolog_, __VA_ARGS__)(cond, ##__VA_ARGS__)

#define _assume_0(cond)      _assume(cond, NULL, true)
#define _assume_1(cond, msg) _assume(cond, msg, true)
#define assume(cond, ...) \
	MACROHAX_OVERLOAD_NARGS(_assume_, __VA_ARGS__)(cond, ##__VA_ARGS__)

#define _assume_nolog_0(cond)      _assume(cond, NULL, false)
#define _assume_nolog_1(cond, msg) _assume(cond, msg, false)
#define assume_nolog(cond, ...) \
	MACROHAX_OVERLOAD_NARGS(_assume_nolog_, __VA_ARGS__)(cond, ##__VA_ARGS__)
