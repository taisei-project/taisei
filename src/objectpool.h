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

#ifdef DEBUG
	#define OBJPOOL_DEBUG
#endif

#ifdef OBJPOOL_DEBUG
	#define OBJPOOL_TRACK_STATS
	#define IF_OBJPOOL_DEBUG(code) code
#else
	#define IF_OBJPOOL_DEBUG(code)
#endif

typedef struct ObjectPool ObjectPool;
typedef struct ObjectPoolStats ObjectPoolStats;

struct ObjectPoolStats {
	const char *tag;
	size_t capacity;
	size_t usage;
	size_t peak_usage;
};

#define OBJPOOL_ALLOC(typename,max_objects) objpool_alloc(sizeof(typename), max_objects, #typename)
#define OBJPOOL_ACQUIRE(pool, type) CASTPTR_ASSUME_ALIGNED(objpool_acquire(pool), type)

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) attr_returns_allocated attr_nonnull(3);
void objpool_free(ObjectPool *pool) attr_nonnull(1);
void *objpool_acquire(ObjectPool *pool) attr_returns_allocated attr_hot attr_nonnull(1);
void objpool_release(ObjectPool *pool, void *object) attr_hot attr_nonnull(1, 2);
void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) attr_nonnull(1, 2);
size_t objpool_object_size(ObjectPool *pool) attr_nonnull(1);

#ifdef OBJPOOL_DEBUG
void objpool_memtest(ObjectPool *pool, void *object) attr_nonnull(1, 2);
#else
#define objpool_memtest(pool, object) ((void)(pool), (void)(object))
#endif
