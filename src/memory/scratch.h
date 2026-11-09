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

typedef struct ScratchGuard {
	MemArena *arena;
	bool pass;
} ScratchGuard;

INLINE void _scratch_guard_cleanup(ScratchGuard *_sg) {
	release_scratch_arena(_sg->arena);
}

/*
 * Expression form:
 *
 * auto foo = WITH_SCRATCH(s, make_foo(s));
 */
#define WITH_SCRATCH_1(_scratch, ...) ({ \
	attr_cleanup(_scratch_guard_cleanup) ScratchGuard _sg = { acquire_scratch_arena() }; \
	auto _scratch = _sg.arena; \
	__VA_ARGS__; \
})

/*
 * Statement form:
 *
 * WITH_SCRATCH(s) {
 * 	if(!do_foo(s)) {
 * 		return;  // returns are fine
 * 	}
 *
 * 	do_bar(s);
 * }
 *
 * WARNING: break and continue will not behave as expected. non-returning calls will leak the arena
 */
#define WITH_SCRATCH_0(_scratch) \
	for(attr_cleanup(_scratch_guard_cleanup) ScratchGuard _sg = { acquire_scratch_arena(), true }; \
		_sg.pass; _sg.pass = false) \
	for(MemArena *_scratch = _sg.arena; _scratch; _scratch = NULL)

#define WITH_SCRATCH(_scratch, ...) \
	MACROHAX_OVERLOAD_HASARGS(WITH_SCRATCH_, __VA_ARGS__)(_scratch, ##__VA_ARGS__)
