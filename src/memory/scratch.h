/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "arena.h"
#include "util/macrohax.h"

MemArena *acquire_scratch_arena(void)
	attr_returns_allocated;

void release_scratch_arena(MemArena *arena)
	attr_nonnull_all;

/*
 * Expression form:
 *
 * auto foo = WITH_SCRATCH(s, make_foo(s));
 */
#define WITH_SCRATCH_1(_scratch, ...) ({ \
	auto _scratch = acquire_scratch_arena(); \
	auto _result = __VA_ARGS__; \
	release_scratch_arena(_scratch); \
	_result; \
})

/*
 * Statement form:
 *
 * WITH_SCRATCH(s) {
 * 	do_foo(s);
 * }
 */
#define WITH_SCRATCH_0(_scratch) \
	for(MemArena *_scratch = acquire_scratch_arena(); _scratch; release_scratch_arena(_scratch), _scratch = NULL)

#define WITH_SCRATCH(_scratch, ...) \
	MACROHAX_OVERLOAD_HASARGS(WITH_SCRATCH_, __VA_ARGS__)(_scratch, ##__VA_ARGS__)
