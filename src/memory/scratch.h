/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "arena.h"

MemArena *acquire_scratch_arena(void)
	attr_returns_allocated;

void release_scratch_arena(MemArena *arena)
	attr_nonnull_all;
