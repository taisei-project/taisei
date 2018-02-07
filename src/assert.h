/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

// NOTE: This file intentionally shadows the standard header!

#include <stdnoreturn.h>
#include <stdbool.h>

noreturn void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log);

#undef assert
#undef static_assert

#define static_assert _Static_assert

#ifdef NDEBUG
    #define _assert(cond,uselog)
#else
    #define _assert(cond,uselog) ((cond) ? (void)0 : _ts_assert_fail(#cond, __func__, __FILE__, __LINE__, uselog))
#endif

#define assert(cond) _assert(cond, true)
#define assert_nolog(cond) _assert(cond, false)
