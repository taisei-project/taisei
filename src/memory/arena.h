/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "list.h"

typedef struct MemArena MemArena;
typedef struct MemArenaPage MemArenaPage;

struct MemArena {
	LIST_ANCHOR(MemArenaPage) pages;
	size_t page_offset;
	size_t total_used;
	size_t total_allocated;
};

struct MemArenaPage {
	LIST_INTERFACE(MemArenaPage);
	MemArena *arena;
	size_t size;
	alignas(alignof(max_align_t)) char data[];
};

void marena_init(MemArena *arena, size_t min_size)
	attr_nonnull_all;

void marena_deinit(MemArena *arena)
	attr_nonnull_all;

void marena_reset(MemArena *arena)
	attr_nonnull_all;

void *marena_alloc(MemArena *arena, size_t size)
	attr_alloc_size(2)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *marena_alloc_array(MemArena *arena, size_t num_members, size_t size)
	attr_alloc_size(2, 3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *marena_alloc_aligned(MemArena *arena, size_t size, size_t alignment)
	attr_alloc_size(2)
	attr_alloc_align(3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;
