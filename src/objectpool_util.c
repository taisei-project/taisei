/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "objectpool_util.h"

static void* objpool_release_list_callback(List **dest, List *elem, void *vpool) {
    list_unlink(dest, elem);
    objpool_release((ObjectPool*)vpool, (ObjectInterface*)elem);
    return NULL;
}

void objpool_release_list(ObjectPool *pool, List **dest) {
    list_foreach(dest, objpool_release_list_callback, pool);
}

bool objpool_is_full(ObjectPool *pool) {
    ObjectPoolStats stats;
    objpool_get_stats(pool, &stats);
    return stats.capacity == stats.usage;
}
