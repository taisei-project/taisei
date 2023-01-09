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
#include "list.h"

typedef struct ObjHeader {
	alignas(alignof(max_align_t)) struct ObjHeader *next;
} ObjHeader;

struct ObjectPool {
	char *tag;
	size_t size_of_object;
	size_t max_objects;
#ifdef OBJPOOL_TRACK_STATS
	size_t usage;
	size_t peak_usage;
#endif
	size_t num_extents;
	char **extents;
	ObjHeader *free_objects;
	char objects[];
};

INLINE attr_returns_max_aligned
ObjHeader *obj_ptr(ObjectPool *pool, char *objects, size_t idx) {
	return CASTPTR_ASSUME_ALIGNED(objects + idx * pool->size_of_object, ObjHeader);
}

static void objpool_register_objects(ObjectPool *pool, char *objects) {
	for(size_t i = 0; i < pool->max_objects; ++i) {
		ObjHeader *o = obj_ptr(pool, objects, i);
		o->next = pool->free_objects;
		pool->free_objects = o;
	}
}

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) {
	// TODO: overflow handling

	auto pool = ALLOC_FLEX(ObjectPool, obj_size * max_objects);
	pool->size_of_object = obj_size;
	pool->max_objects = max_objects;
	pool->tag = strdup(tag);

	objpool_register_objects(pool, pool->objects);

	log_debug("[%s] Allocated pool for %zu objects, %zu bytes each",
		pool->tag,
		pool->max_objects,
		pool->size_of_object
	);

	return pool;
}

static char *objpool_add_extent(ObjectPool *pool) {
	pool->extents = mem_realloc(pool->extents, (++pool->num_extents) * sizeof(*pool->extents));
	char *extent = pool->extents[pool->num_extents - 1] = mem_alloc_array(pool->max_objects, pool->size_of_object);
	objpool_register_objects(pool, extent);
	return extent;
}

static char* objpool_fmt_size(ObjectPool *pool) {
	switch(pool->num_extents) {
		case 0:
			return strfmt("%zu objects, %zu bytes each",
				pool->max_objects,
				pool->size_of_object
			);

		case 1:
			return strfmt("%zu objects, %zu bytes each, with 1 extent",
				pool->max_objects * 2,
				pool->size_of_object
			);

		default:
			return strfmt("%zu objects, %zu bytes each, with %zu extents",
				pool->max_objects * (1 + pool->num_extents),
				pool->size_of_object,
				pool->num_extents
			);
	}
}

void *objpool_acquire(ObjectPool *pool) {
	ObjHeader *obj = pool->free_objects;

	if(obj) {
acquired:
		pool->free_objects = obj->next;
		memset(obj, 0, pool->size_of_object);

#ifdef OBJPOOL_TRACK_STATS
		if(++pool->usage > pool->peak_usage) {
			pool->peak_usage = pool->usage;
		}
#endif

		return obj;
	}

	char *tmp = objpool_fmt_size(pool);
	log_debug("[%s] Object pool exhausted (%s), extending",
		pool->tag,
		tmp
	);
	mem_free(tmp);

	objpool_add_extent(pool);
	obj = pool->free_objects;
	assert(obj != NULL);
	goto acquired;
}

void objpool_release(ObjectPool *pool, void *object) {
	objpool_memtest(pool, object);
	ObjHeader *obj = object;
	obj->next = pool->free_objects;
	pool->free_objects = obj;
#ifdef OBJPOOL_TRACK_STATS
	pool->usage--;
#endif
}

void objpool_free(ObjectPool *pool) {
#ifdef OBJPOOL_TRACK_STATS
	if(pool->usage != 0) {
		log_warn("[%s] %zu objects still in use", pool->tag, pool->usage);
	}
#endif

	for(size_t i = 0; i < pool->num_extents; ++i) {
		mem_free(pool->extents[i]);
	}

	mem_free(pool->extents);
	mem_free(pool->tag);
	mem_free(pool);
}

size_t objpool_object_size(ObjectPool *pool) {
	return pool->size_of_object;
}

void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) {
	stats->tag = pool->tag;
	stats->capacity = pool->max_objects * (1 + pool->num_extents);
#ifdef OBJPOOL_TRACK_STATS
	stats->usage = pool->usage;
	stats->peak_usage = pool->peak_usage;
#else
	stats->usage = 0;
	stats->peak_usage = 0;
#endif
}

attr_unused
static bool objpool_object_in_subpool(ObjectPool *pool, ObjHeader *object, char *objects) {
	char *objofs = (char*)object;
	char *minofs = objects;
	char *maxofs = objects + (pool->max_objects - 1) * pool->size_of_object;

	if(objofs < minofs || objofs > maxofs) {
		return false;
	}

	ptrdiff_t misalign = (ptrdiff_t)(objofs - objects) % pool->size_of_object;

	if(misalign) {
		log_fatal("[%s] Object pointer %p is misaligned by %zi",
			pool->tag,
			(void*)objofs,
			(ssize_t)misalign
		);
	}

	return true;
}

attr_unused
static bool objpool_object_in_pool(ObjectPool *pool, ObjHeader *object) {
	if(objpool_object_in_subpool(pool, object, pool->objects)) {
		return true;
	}

	for(size_t i = 0; i < pool->num_extents; ++i) {
		if(objpool_object_in_subpool(pool, object, pool->extents[i])) {
			return true;
		}
	}

	return false;
}

#ifdef OBJPOOL_DEBUG
void objpool_memtest(ObjectPool *pool, void *object) {
	if(!objpool_object_in_pool(pool, object)) {
		log_fatal("[%s] Object pointer %p does not belong to this pool",
			pool->tag,
			object
		);
	}
}
#endif
