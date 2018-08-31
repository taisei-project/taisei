/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "objectpool_util.h"
#include "util.h"

bool objpool_is_full(ObjectPool *pool) {
	ObjectPoolStats stats;
	objpool_get_stats(pool, &stats);
	return stats.capacity == stats.usage;
}

size_t objpool_object_contents_size(ObjectPool *pool) {
	return objpool_object_size(pool) - sizeof(ObjectInterface);
}

void *objpool_object_contents(ObjectPool *pool, ObjectInterface *obj, size_t *out_size) {
	assert(obj != NULL);

	if(out_size != NULL) {
		objpool_memtest(pool, obj);
		*out_size = objpool_object_contents_size(pool);
	}

	return (char*)obj + sizeof(ObjectInterface);
}
