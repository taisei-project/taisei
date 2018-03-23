/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "objectpool.h"
#include "util.h"
#include "list.h"

struct ObjectPool {
	char *tag;
	size_t size_of_object;
	size_t max_objects;
	size_t usage;
	size_t peak_usage;
	size_t num_extents;
	char **extents;
	ObjectInterface *free_objects;
	char objects[];
};

static inline ObjectInterface* obj_ptr(ObjectPool *pool, char *objects, size_t idx) {
	return (ObjectInterface*)(void*)(objects + idx * pool->size_of_object);
}

static void objpool_register_objects(ObjectPool *pool, char *objects) {
	for(size_t i = 0; i < pool->max_objects; ++i) {
		list_push(&pool->free_objects, obj_ptr(pool, objects, i));
	}
}

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) {
	// TODO: overflow handling

	ObjectPool *pool = malloc(sizeof(ObjectPool) + (obj_size * max_objects));
	pool->size_of_object = obj_size;
	pool->max_objects = max_objects;
	pool->free_objects = NULL;
	pool->usage = 0;
	pool->peak_usage = 0;
	pool->num_extents = 0;
	pool->extents = NULL;
	pool->tag = strdup(tag);

	memset(pool->objects, 0, obj_size * max_objects);
	objpool_register_objects(pool, pool->objects);

	log_debug("[%s] Allocated pool for %zu objects, %zu bytes each",
		pool->tag,
		pool->max_objects,
		pool->size_of_object
	);

	return pool;
}

static char *objpool_add_extent(ObjectPool *pool) {
	pool->extents = realloc(pool->extents, (++pool->num_extents) * sizeof(pool->extents));
	char *extent = pool->extents[pool->num_extents - 1] = calloc(pool->max_objects, pool->size_of_object);
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

ObjectInterface *objpool_acquire(ObjectPool *pool) {
	ObjectInterface *obj = list_pop(&pool->free_objects);

	if(obj) {
acquired:
		memset(obj, 0, pool->size_of_object);

		IF_OBJPOOL_DEBUG({
			obj->_object_private.used = true;
		})

		if(++pool->usage > pool->peak_usage) {
			pool->peak_usage = pool->usage;
		}

		// log_debug("[%s] Usage: %zu", pool->tag, pool->usage);
		return obj;
	}

	char *tmp = objpool_fmt_size(pool);
	log_warn("[%s] Object pool exhausted (%s), extending",
		pool->tag,
		tmp
	);
	free(tmp);

	objpool_add_extent(pool);
	obj = list_pop(&pool->free_objects);
	assert(obj != NULL);
	goto acquired;
}

void objpool_release(ObjectPool *pool, ObjectInterface *object) {
	objpool_memtest(pool, object);

	IF_OBJPOOL_DEBUG({
		if(!object->_object_private.used) {
			log_fatal("[%s] Attempted to release an unused object %p",
				pool->tag,
				(void*)object
			);
		}

		object->_object_private.used = false;
	})

	list_push(&pool->free_objects, object);

	pool->usage--;
	// log_debug("[%s] Usage: %zu", pool->tag, pool->usage);
}

void objpool_free(ObjectPool *pool) {
	if(!pool) {
		return;
	}

	if(pool->usage != 0) {
		log_warn("[%s] %zu objects still in use", pool->tag, pool->usage);
	}

	for(size_t i = 0; i < pool->num_extents; ++i) {
		free(pool->extents[i]);
	}

	free(pool->extents);
	free(pool->tag);
	free(pool);
}

size_t objpool_object_size(ObjectPool *pool) {
	return pool->size_of_object;
}

void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) {
	stats->tag = pool->tag;
	stats->capacity = pool->max_objects * (1 + pool->num_extents);
	stats->usage = pool->usage;
	stats->peak_usage = pool->peak_usage;
}

attr_unused
static bool objpool_object_in_subpool(ObjectPool *pool, ObjectInterface *object, char *objects) {
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
static bool objpool_object_in_pool(ObjectPool *pool, ObjectInterface *object) {
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

void objpool_memtest(ObjectPool *pool, ObjectInterface *object) {
	assert(pool != NULL);
	assert(object != NULL);

	IF_OBJPOOL_DEBUG({
		if(!objpool_object_in_pool(pool, object)) {
			log_fatal("[%s] Object pointer %p does not belong to this pool",
				pool->tag,
				(void*)object
			);
		}
	})
}
