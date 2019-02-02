/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_util_assert_h
#define IGUARD_util_assert_h

#include "taisei.h"

noreturn void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log);

#undef assert
#undef static_assert

#define static_assert _Static_assert

#ifdef NDEBUG
    #define _assert(cond, uselog)
	#define _assume(cond, uselog) ASSUME(cond)
#else
    #define _assert(cond, uselog) ((cond) ? (void)0 : _ts_assert_fail(#cond, __func__, __FILE__, __LINE__, uselog))
	#define _assume(cond, uselog) _assert(cond, uselog)
#endif

#define assert(cond) _assert(cond, true)
#define assert_nolog(cond) _assert(cond, false)

#define assume(cond) _assume(cond, true)
#define assume_nolog(cond) _assume(cond, false)

#endif // IGUARD_util_assert_h
