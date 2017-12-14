
#include "objectpool.h"
#include "util.h"
#include "list.h"

#ifdef OBJPOOL_DEBUG
    #define OBJ_USED(pool,obj) ((pool)->usemap[((uintptr_t)(obj) - (uintptr_t)(pool)->objects) / (pool)->size_of_object])
#endif

struct ObjectPool {
    char *tag;
    size_t size_of_object;
    size_t max_objects;
    size_t usage;
    size_t peak_usage;
    ObjectInterface *free_objects;

    IF_OBJPOOL_DEBUG(
        bool *usemap;
    )

    char objects[];
};

static inline ObjectInterface* obj_ptr(ObjectPool *pool, size_t idx) {
    return (ObjectInterface*)(pool->objects + idx * pool->size_of_object);
}

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag) {
    // TODO: overflow handling

    ObjectPool *pool = malloc(sizeof(ObjectPool) + (obj_size * max_objects));
    pool->size_of_object = obj_size;
    pool->max_objects = max_objects;
    pool->free_objects = NULL;
    pool->usage = 0;
    pool->peak_usage = 0;
    pool->tag = strdup(tag);

    IF_OBJPOOL_DEBUG({
        pool->usemap = calloc(max_objects, sizeof(bool));
    })

    memset(pool->objects, 0, obj_size * max_objects);

    for(size_t i = 0; i < max_objects; ++i) {
        list_push((List**)&pool->free_objects, (List*)obj_ptr(pool, i));
    }

    log_debug("[%s] Allocated pool for %zu objects, %zu bytes each",
        pool->tag,
        pool->max_objects,
        pool->size_of_object
    );

    return pool;
}

ObjectInterface *objpool_acquire(ObjectPool *pool) {
    ObjectInterface *obj = (ObjectInterface*)list_pop((List**)&pool->free_objects);

    if(obj) {
        memset(obj, 0, pool->size_of_object);

        IF_OBJPOOL_DEBUG({
            OBJ_USED(pool, obj) = true;
        })

        if(++pool->usage > pool->peak_usage) {
            pool->peak_usage = pool->usage;
        }

        // log_debug("[%s] Usage: %zu", pool->tag, pool->usage);
        return obj;
    }

    log_fatal("[%s] Object pool exhausted (%zu objects, %zu bytes each)",
        pool->tag,
        pool->max_objects,
        pool->size_of_object
    );
}

void objpool_release(ObjectPool *pool, ObjectInterface *object) {
    IF_OBJPOOL_DEBUG({
        char *objofs = (char*)object;
        char *minofs = pool->objects;
        char *maxofs = pool->objects + (pool->max_objects - 1) * pool->size_of_object;

        if(objofs < minofs || objofs > maxofs) {
            log_fatal("[%s] Object pointer %p is not within range %p - %p",
                pool->tag,
                (void*)objofs,
                (void*)minofs,
                (void*)maxofs
            );
        }

        ptrdiff_t misalign = (ptrdiff_t)(objofs - pool->objects) % pool->size_of_object;

        if(misalign) {
            log_fatal("[%s] Object pointer %p is misaligned by %zi",
                pool->tag,
                (void*)objofs,
                (ssize_t)misalign
            );
        }

        if(!OBJ_USED(pool, object)) {
            log_fatal("[%s] Attempted to release an unused object %p",
                pool->tag,
                (void*)objofs
            );
        }

        OBJ_USED(pool, object) = false;
    })

    list_push((List**)&pool->free_objects, (List*)object);
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

    IF_OBJPOOL_DEBUG({
        free(pool->usemap);
    })

    free(pool->tag);
    free(pool);
}

void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats) {
    stats->tag = pool->tag;
    stats->capacity = pool->max_objects;
    stats->usage = pool->usage;
    stats->peak_usage = pool->peak_usage;
}
