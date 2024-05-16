/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "allocator.h"
#include "util.h"
#include "../util.h"

static void *allocator_default_alloc(Allocator *alloc, size_t size) {
	return mem_alloc(size);
}

static void *allocator_default_alloc_aligned(Allocator *alloc, size_t size, size_t align) {
	return mem_alloc_aligned(size, align);
}

static void allocator_default_free(Allocator *alloc, void *mem) {
	mem_free(mem);
}

Allocator default_allocator = {
	.procs = {
		.alloc = allocator_default_alloc,
		.alloc_aligned = allocator_default_alloc_aligned,
		.free = allocator_default_free,
	}
};

void *allocator_alloc(Allocator *alloc, size_t size) {
	return NOT_NULL(alloc->procs.alloc)(alloc, size);
}

void *allocator_alloc_array(Allocator *alloc, size_t num_members, size_t size) {
	return allocator_alloc(alloc, mem_util_calc_array_size(num_members, size));
}

void *allocator_alloc_aligned(Allocator *alloc, size_t size, size_t alignment) {
	if(UNLIKELY(alloc->procs.alloc_aligned == NULL)) {
		log_fatal("alloc_aligned not implemented by allocator");
	}

	return alloc->procs.alloc_aligned(alloc, size, alignment);
}

void *allocator_alloc_array_aligned(Allocator *alloc, size_t num_members, size_t size, size_t alignment) {
	return allocator_alloc_aligned(alloc, mem_util_calc_array_size(num_members, size), alignment);
}

void allocator_free(Allocator *alloc, void *mem) {
	return NOT_NULL(alloc->procs.free)(alloc, mem);
}

void allocator_deinit(Allocator *alloc) {
	if(alloc->procs.deinit) {
		alloc->procs.deinit(alloc);
	}
}

static void *allocator_arena_alloc(Allocator *alloc, size_t size) {
	return marena_alloc(NOT_NULL(alloc->userdata), size);
}

static void *allocator_arena_alloc_aligned(Allocator *alloc, size_t size, size_t alignment) {
	return marena_alloc_aligned(NOT_NULL(alloc->userdata), size, alignment);
}

static void allocator_arena_free(Allocator *alloc, void *mem) {
	// arena allocations can't be freed
}

void allocator_init_from_arena(Allocator *alloc, MemArena *arena) {
	*alloc = (Allocator) {
		.userdata = arena,
		.procs = {
			.alloc = allocator_arena_alloc,
			.alloc_aligned = allocator_arena_alloc_aligned,
			.free = allocator_arena_free,
		}
	};
}
