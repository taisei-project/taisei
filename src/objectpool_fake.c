/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "objectpool.h"
#include "util.h"

struct ObjectPool {
    size_t size_of_object;
};

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) {
    ObjectPool *pool = malloc(sizeof(ObjectPool));
    pool->size_of_object = obj_size;
    return pool;
}

ObjectInterface *objpool_acquire(ObjectPool *pool) {
    return calloc(1, pool->size_of_object);
}

void objpool_release(ObjectPool *pool, ObjectInterface *object) {
    free(object);
}

void objpool_free(ObjectPool *pool) {
    free(pool);
}

void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) {
    memset(&stats, 0, sizeof(ObjectPoolStats));
    stats->tag = "<N/A>";
}
