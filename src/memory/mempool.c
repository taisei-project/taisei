/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "mempool.h"

void mempool_generic_init(
	MemPool *pool,
	MemArena *arena
) {
	*pool = (MemPool) {
		.arena = arena,
	};
}

void *mempool_generic_acquire(MemPool *pool, size_t size, size_t align) {
	MemPoolObjectHeader *obj = pool->free_objects.as_generic;

	if(obj) {
		assert(pool->num_used < pool->num_allocated);
		pool->free_objects.as_generic = obj->next;
	} else {
		assert(pool->num_used == pool->num_allocated);
		obj = marena_alloc_aligned(pool->arena, size, align);
		++pool->num_allocated;
	}

	++pool->num_used;

	assert(pool->num_used <= pool->num_allocated);
	return obj;
}

void mempool_generic_release(MemPool *pool, void *object) {
	MemPoolObjectHeader *obj = object;
	obj->next = pool->free_objects.as_generic;
	pool->free_objects.as_generic = obj;

	--pool->num_used;
	assert(pool->num_used >= 0);
	assert(pool->num_used <= pool->num_allocated);
}
