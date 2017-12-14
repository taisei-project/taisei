
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
