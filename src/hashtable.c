/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "hashtable.h"
#include "list.h"
#include "util.h"

#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <SDL_mutex.h>

typedef struct HashtableElement {
    void *next;
    void *prev;
    void *data;
    void *key;
    hash_t hash;
} HashtableElement;

struct Hashtable {
    HashtableElement **table;
    size_t table_size;
    HTCmpFunc cmp_func;
    HTHashFunc hash_func;
    HTCopyFunc copy_func;
    HTFreeFunc free_func;
    SDL_atomic_t cur_operation;
    SDL_atomic_t num_operations;
    size_t num_elements;
    bool dynamic_size;
};

enum {
    HT_OP_NONE,
    HT_OP_READ,
    HT_OP_WRITE,
};

typedef struct HashtableIterator {
    Hashtable *hashtable;
    size_t bucketnum;
    HashtableElement *elem;
} HashtableIterator;

/*
 *  Generic functions
 */

static size_t constraint_size(size_t size) {
    if(size < HT_MIN_SIZE)
        return HT_MIN_SIZE;

    if(size > HT_MAX_SIZE)
        return HT_MAX_SIZE;

    return size;
}

Hashtable* hashtable_new(size_t size, HTCmpFunc cmp_func, HTHashFunc hash_func, HTCopyFunc copy_func, HTFreeFunc free_func) {
    Hashtable *ht = malloc(sizeof(Hashtable));

    if(size == HT_DYNAMIC_SIZE) {
        size = constraint_size(0);
        ht->dynamic_size = true;
    } else {
        size = constraint_size(size);
        ht->dynamic_size = false;
    }

    if(!cmp_func) {
        cmp_func = hashtable_cmpfunc_ptr;
    }

    if(!copy_func) {
        copy_func = hashtable_copyfunc_ptr;
    }

    ht->table = calloc(size, sizeof(HashtableElement*));
    ht->table_size = size;
    ht->num_elements = 0;
    ht->cmp_func = cmp_func;
    ht->hash_func = hash_func;
    ht->copy_func = copy_func;
    ht->free_func = free_func;

    SDL_AtomicSet(&ht->cur_operation, HT_OP_NONE);
    SDL_AtomicSet(&ht->num_operations, 0);

    assert(ht->hash_func != NULL);

    return ht;
}

static bool hashtable_try_enter_state(Hashtable *ht, int state, bool mutex) {
    SDL_atomic_t *ptr = &ht->cur_operation;

    if(!mutex) {
        SDL_AtomicCAS(ptr, state, HT_OP_NONE);
    }

    return SDL_AtomicCAS(ptr, HT_OP_NONE, state);
}

static void hashtable_enter_state(Hashtable *ht, int state, bool mutex) {
    while(!hashtable_try_enter_state(ht, state, mutex) || (mutex && SDL_AtomicGet(&ht->num_operations)));
    SDL_AtomicIncRef(&ht->num_operations);
}

static void hashtable_idle_state(Hashtable *ht) {
    if(SDL_AtomicDecRef(&ht->num_operations))
        SDL_AtomicSet(&ht->cur_operation, HT_OP_NONE);
}

void hashtable_lock(Hashtable *ht) {
    assert(ht != NULL);
    hashtable_enter_state(ht, HT_OP_READ, false);
}

void hashtable_unlock(Hashtable *ht) {
    assert(ht != NULL);
    hashtable_idle_state(ht);
}

static void hashtable_delete_callback(void **vlist, void *velem, void *vht) {
    Hashtable *ht = vht;
    HashtableElement *elem = velem;

    if(ht->free_func) {
        ht->free_func(elem->key);
    }

    delete_element(vlist, velem);
}

static void hashtable_unset_all_internal(Hashtable *ht) {
    for(size_t i = 0; i < ht->table_size; ++i) {
        delete_all_elements_witharg((void**)(ht->table + i), hashtable_delete_callback, ht);
    }

    ht->num_elements = 0;
}

void hashtable_unset_all(Hashtable *ht) {
    assert(ht != NULL);
    hashtable_enter_state(ht, HT_OP_WRITE, true);
    hashtable_unset_all_internal(ht);
    hashtable_idle_state(ht);
}

void hashtable_free(Hashtable *ht) {
    if(!ht) {
        return;
    }

    hashtable_unset_all(ht);

    free(ht->table);
    free(ht);
}

void* hashtable_get(Hashtable *ht, void *key) {
    assert(ht != NULL);

    hash_t hash = ht->hash_func(key);
    hashtable_enter_state(ht, HT_OP_READ, false);
    HashtableElement *elems = ht->table[hash % ht->table_size];

    for(HashtableElement *e = elems; e; e = e->next) {
        if(hash == e->hash && ht->cmp_func(key, e->key)) {
            hashtable_idle_state(ht);
            return e->data;
        }
    }

    hashtable_idle_state(ht);
    return NULL;
}

static bool hashtable_set_internal(Hashtable *ht, HashtableElement **table, size_t table_size, hash_t hash, void *key, void *data) {
    bool collisions_updated = false;
    size_t idx = hash % table_size;

    HashtableElement *elems = table[idx], *elem;

    for(HashtableElement *e = elems; e; e = e->next) {
        if(hash == e->hash && ht->cmp_func(key, e->key)) {
            if(ht->free_func) {
                ht->free_func(e->key);
            }

            delete_element((void**)&elems, e);
            ht->num_elements--;

            if(elems) {
                // we have just deleted an element from a bucket that had collisions.
                collisions_updated = true;
            }

            break;
        }
    }

    if(data) {
        if(elems) {
            // we are adding an element to a non-empty bucket.
            // normally this is a new collision, unless we are replacing an existing element.
            collisions_updated = !collisions_updated;
        }

        elem = create_element((void**)&elems, sizeof(HashtableElement));
        ht->copy_func(&elem->key, key);
        elem->hash = ht->hash_func(elem->key);
        elem->data = data;

        ht->num_elements++;
    }

    table[idx] = elems;
    return collisions_updated;
}

static int hashtable_check_collisions_with_new_size(Hashtable *ht, size_t size) {
    int *ctable = calloc(sizeof(int), size);
    int cols = 0;

    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            size_t idx = e->hash % size;
            ++ctable[idx];

            if(ctable[idx] > 1) {
                ++cols;
            }
        }
    }

    free(ctable);
    return cols;
}

static size_t hashtable_find_optimal_size(Hashtable *ht) {
    size_t best_size = ht->table_size;
    int col_tolerance = min(HT_COLLISION_TOLERANCE + 1, max(ht->num_elements / 2, 1));
    int min_cols = 0;

    for(size_t s = best_size; s < HT_MAX_SIZE; s += HT_RESIZE_STEP) {
        int cols = hashtable_check_collisions_with_new_size(ht, s);

        if(cols < col_tolerance) {
            log_debug("Optimal size for %p is %"PRIuMAX" (%i collisions)", (void*)ht, (uintmax_t)s, cols);
            return s;
        }

        if(cols < min_cols) {
            min_cols = cols;
            best_size = s;
        }
    }

    log_debug("Optimal size for %p is %"PRIuMAX" (%i collisions)", (void*)ht, (uintmax_t)best_size, min_cols);
    return best_size;
}

static void hashtable_resize_internal(Hashtable *ht, size_t new_size) {
    new_size = constraint_size(new_size);

    if(new_size == ht->table_size) {
        return;
    }

    HashtableElement **new_table = calloc(new_size, sizeof(HashtableElement*));

    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            hashtable_set_internal(ht, new_table, new_size, e->hash, e->key, e->data);
        }
    }

    hashtable_unset_all_internal(ht);
    free(ht->table);

    ht->table = new_table;

    log_debug("Resized hashtable at %p: %"PRIuMAX" -> %"PRIuMAX"",
        (void*)ht, (uintmax_t)ht->table_size, (uintmax_t)new_size);

    ht->table_size = new_size;
}

void hashtable_resize(Hashtable *ht, size_t new_size) {
    hashtable_enter_state(ht, HT_OP_WRITE, true);
    hashtable_resize_internal(ht, new_size);
    hashtable_idle_state(ht);
}

void hashtable_set(Hashtable *ht, void *key, void *data) {
    assert(ht != NULL);

    hash_t hash = ht->hash_func(key);

    hashtable_enter_state(ht, HT_OP_WRITE, true);
    bool update = hashtable_set_internal(ht, ht->table, ht->table_size, hash, key, data);

    if(ht->dynamic_size && update) {
        hashtable_resize_internal(ht, hashtable_find_optimal_size(ht));
    }

    hashtable_idle_state(ht);
}

void hashtable_unset(Hashtable *ht, void *key) {
    hashtable_set(ht, key, NULL);
}

void hashtable_unset_deferred(Hashtable *ht, void *key, ListContainer **list) {
    assert(ht != NULL);
    ListContainer *c = create_element((void**)list, sizeof(ListContainer));
    ht->copy_func(&c->data, key);
}

void hashtable_unset_deferred_now(Hashtable *ht, ListContainer **list) {
    ListContainer *next;
    assert(ht != NULL);

    for(ListContainer *c = *list; c; c = next) {
        next = c->next;
        hashtable_unset(ht, c->data);
        delete_element((void**)list, c);
    }
}

/*
 *  Iteration functions
 */

void* hashtable_foreach(Hashtable *ht, HTIterCallback callback, void *arg) {
    assert(ht != NULL);

    void *ret = NULL;

    hashtable_enter_state(ht, HT_OP_READ, false);

    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            ret = callback(e->key, e->data, arg);
            if(ret) {
                return ret;
            }
        }
    }

    hashtable_idle_state(ht);

    return ret;
}

HashtableIterator* hashtable_iter(Hashtable *ht) {
    assert(ht != NULL);
    HashtableIterator *iter = malloc(sizeof(HashtableIterator));
    iter->hashtable = ht;
    iter->bucketnum = (size_t)-1;
    return iter;
}

bool hashtable_iter_next(HashtableIterator *iter, void **out_key, void **out_data) {
    Hashtable *ht = iter->hashtable;

    if(iter->bucketnum == (size_t)-1) {
        iter->bucketnum = 0;
        iter->elem = ht->table[iter->bucketnum];
    } else {
        iter->elem = iter->elem->next;
    }

    while(!iter->elem) {
        if(++iter->bucketnum == ht->table_size) {
            free(iter);
            return false;
        }

        iter->elem = ht->table[iter->bucketnum];
    }

    if(out_key) {
        *out_key = iter->elem->key;
    }

    if(out_data) {
        *out_data = iter->elem->data;
    }

    return true;
}

/*
 *  Convenience functions for hashtables with string keys
 */

bool hashtable_cmpfunc_string(void *str1, void *str2) {
    return !strcmp((const char*)str1, (const char*)(str2));
}

hash_t hashtable_hashfunc_string(void *vstr) {
    return crc32str(0, (const char*)vstr);
}

void hashtable_copyfunc_string(void **dst, void *src) {
    *dst = malloc(strlen((char*)src) + 1);
    strcpy(*dst, src);
}

// #define hashtable_freefunc_string free

Hashtable* hashtable_new_stringkeys(size_t size) {
    return hashtable_new(size,
        hashtable_cmpfunc_string,
        hashtable_hashfunc_string,
        hashtable_copyfunc_string,
        hashtable_freefunc_string
    );
}

void* hashtable_get_string(Hashtable *ht, const char *key) {
    return hashtable_get(ht, (void*)key);
}

void hashtable_set_string(Hashtable *ht, const char *key, void *data) {
    hashtable_set(ht, (void*)key, data);
}

void hashtable_unset_string(Hashtable *ht, const char *key) {
    hashtable_unset(ht, (void*)key);
}

/*
 *  Misc convenience functions
 */

void* hashtable_iter_free_data(void *key, void *data, void *arg) {
    free(data);
    return NULL;
}

bool hashtable_cmpfunc_ptr(void *p1, void *p2) {
    return p1 == p2;
}

void hashtable_copyfunc_ptr(void **dst, void *src) {
    *dst = src;
}

/*
 *  Diagnostic functions
 */

void hashtable_get_stats(Hashtable *ht, HashtableStats *stats) {
    assert(ht != NULL);
    assert(stats != NULL);

    memset(stats, 0, sizeof(HashtableStats));

    for(size_t i = 0; i < ht->table_size; ++i) {
        int elems = 0;

        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            ++elems;
            ++stats->num_elements;
        }

        if(elems > 1) {
            stats->collisions += elems - 1;
        }

        if(!elems) {
            ++stats->free_buckets;
        } else if(elems > stats->max_per_bucket) {
            stats->max_per_bucket = elems;
        }
    }
}

size_t hashtable_get_approx_overhead(Hashtable *ht) {
    size_t o = sizeof(Hashtable) + sizeof(HashtableElement*) * ht->table_size;

    for(HashtableIterator *i = hashtable_iter(ht); hashtable_iter_next(i, NULL, NULL);) {
        o += sizeof(HashtableElement);
    }

    return o;
}

void hashtable_print_stringkeys(Hashtable *ht) {
    HashtableStats stats;
    hashtable_get_stats(ht, &stats);

    log_debug("------ %p:", (void*)ht);
    for(size_t i = 0; i < ht->table_size; ++i) {
        log_debug("[bucket %"PRIuMAX"] %p", (uintmax_t)i, (void*)ht->table[i]);

        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            log_debug(" -- %s (%"PRIuMAX"): %p", (char*)e->key, (uintmax_t)e->hash, e->data);
        }
    }

    log_debug("%i total elements, %i unused buckets, %i collisions, max %i elems per bucket, %lu approx overhead",
            stats.num_elements, stats.free_buckets, stats.collisions, stats.max_per_bucket,
            (unsigned long int)hashtable_get_approx_overhead(ht));
}

/*
 *  Testing
 */

// #define HASHTABLE_TEST

#ifdef HASHTABLE_TEST

#include <stdio.h>

static void hashtable_printstrings(Hashtable *ht) {
    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            log_info("[HT %"PRIuMAX"] %s (%"PRIuMAX"): %s\n", (uintmax_t)i, (char*)e->key, (uintmax_t)e->hash, (char*)e->data);
        }
    }
}

#endif

int hashtable_test(void) {
#ifdef HASHTABLE_TEST
    const char *mapping[] = {
        "honoka", "umi",
        "test", "12345",
        "herp", "derp",
        "hurr", "durr",
    };

    const size_t elems = sizeof(mapping) / sizeof(char*);
    Hashtable *ht = hashtable_new_stringkeys(128);

    for(int i = 0; i < elems; i += 2) {
        hashtable_set_string(ht, mapping[i], (void*)mapping[i+1]);
    }

    hashtable_printstrings(ht);
    log_info("-----\n");
    hashtable_set_string(ht, "12345", "asdfg");
    hashtable_unset_string(ht, "test");
    hashtable_set_string(ht, "herp", "deeeeeerp");
    hashtable_printstrings(ht);

    hashtable_free(ht);
    return 1;
#else
    return 0;
#endif
}
