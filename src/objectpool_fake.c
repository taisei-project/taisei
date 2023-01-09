/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "objectpool.h"
#include "util.h"

struct ObjectPool {
	size_t size_of_object;
};

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) {
	return ALLOC(ObjectPool, { .size_of_object = obj_size });
}

void *objpool_acquire(ObjectPool *pool) {
	return mem_alloc(pool->size_of_object);
}

void objpool_release(ObjectPool *pool, void *object) {
	mem_free(object);
}

void objpool_free(ObjectPool *pool) {
	mem_free(pool);
}

void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) {
	memset(stats, 0, sizeof(ObjectPoolStats));
	stats->tag = "<N/A>";
}

size_t objpool_object_size(ObjectPool *pool) {
	return pool->size_of_object;
}

#ifdef OBJPOOL_DEBUG
void objpool_memtest(ObjectPool *pool, void *object) {
}
#endif
