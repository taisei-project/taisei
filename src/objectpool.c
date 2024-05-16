/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "objectpool.h"
#include "util.h"
#include "list.h"
#include "memory/arena.h"

void objpool_init(
	ObjectPool *pool,
	const char *tag,
	MemArena *arena,
	size_t obj_size,
	size_t obj_align
) {
	*pool = (ObjectPool) {
		.arena = arena,
		.obj_size = obj_size,
		.obj_align = obj_align,
		.tag = tag,
	};
}

void *objpool_acquire(ObjectPool *pool) {
	ObjHeader *obj = pool->free_objects;

	if(obj) {
		assert(pool->num_used < pool->num_allocated);
		pool->free_objects = obj->next;
	} else {
		assert(pool->num_used == pool->num_allocated);
		obj = marena_alloc_aligned(pool->arena, pool->obj_size, pool->obj_align);
		++pool->num_allocated;
	}

	memset(obj, 0, pool->obj_size);
	++pool->num_used;

	assert(pool->num_used <= pool->num_allocated);
	return obj;
}

void objpool_release(ObjectPool *pool, void *object) {
	ObjHeader *obj = object;
	obj->next = pool->free_objects;
	pool->free_objects = obj;

	--pool->num_used;
	assert(pool->num_used >= 0);
	assert(pool->num_used <= pool->num_allocated);
}
