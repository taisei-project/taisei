/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "arena.h"

/*
 * WARNING: Unlike the memory.h API, the allocator API is currently not guaranteed to return
 * zero-initialized memory.
 */

typedef struct Allocator Allocator;

struct Allocator {
	void *userdata;
	struct {
		void *(*alloc)(Allocator *alloc, size_t size);
		void *(*alloc_aligned)(Allocator *alloc, size_t size, size_t align);
		void (*free)(Allocator *alloc, void *mem);
		void (*deinit)(Allocator *alloc);
	} procs;
};

extern Allocator default_allocator;

void *allocator_alloc(Allocator *alloc, size_t size)
	attr_alloc_size(2)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *allocator_alloc_array(Allocator *alloc, size_t num_members, size_t size)
	attr_alloc_size(2, 3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *allocator_alloc_aligned(Allocator *alloc, size_t size, size_t alignment)
	attr_alloc_size(2)
	attr_alloc_align(3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void allocator_free(Allocator *alloc, void *mem)
	attr_nonnull(1);

void allocator_deinit(Allocator *alloc)
	attr_nonnull_all;

void allocator_init_from_arena(Allocator *alloc, MemArena *arena)
	attr_nonnull_all;
