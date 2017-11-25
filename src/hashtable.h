/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "list.h"

#define HT_MIN_SIZE 15
#define HT_MAX_SIZE 4095
#define HT_COLLISION_TOLERANCE 5
#define HT_DYNAMIC_SIZE 0
#define HT_RESIZE_STEP 1

typedef struct Hashtable Hashtable;
typedef struct HashtableIterator HashtableIterator;
typedef struct HashtableStats HashtableStats;
typedef uint32_t hash_t;

struct HashtableStats {
    unsigned int free_buckets;
    unsigned int collisions;
    unsigned int max_per_bucket;
    unsigned int num_elements;
};

typedef bool (*HTCmpFunc)(void *key1, void *key2);
typedef hash_t (*HTHashFunc)(void *key);
typedef void (*HTCopyFunc)(void **dstkey, void *srckey);
typedef void (*HTFreeFunc)(void *key);
typedef void* (*HTIterCallback)(void *key, void *data, void *arg);

Hashtable* hashtable_new(size_t size, HTCmpFunc cmp_func, HTHashFunc hash_func, HTCopyFunc copy_func, HTFreeFunc free_func);
void hashtable_free(Hashtable *ht);
void* hashtable_get(Hashtable *ht, void *key) __attribute__((hot));
void* hashtable_get_unsafe(Hashtable *ht, void *key) __attribute__((hot));
void hashtable_set(Hashtable *ht, void *key, void *data);
void hashtable_unset(Hashtable *ht, void *key);
void hashtable_unset_deferred(Hashtable *ht, void *key, ListContainer **list);
void hashtable_unset_deferred_now(Hashtable *ht, ListContainer **list);
void hashtable_unset_all(Hashtable *ht);

void* hashtable_foreach(Hashtable *ht, HTIterCallback callback, void *arg);

// THIS IS NOT THREAD-SAFE. You have to use hashtable_lock/unlock to make it so.
HashtableIterator* hashtable_iter(Hashtable *ht);
bool hashtable_iter_next(HashtableIterator *iter, void **out_key, void **out_data);

bool hashtable_cmpfunc_string(void *str1, void *str2) __attribute__((hot));
hash_t hashtable_hashfunc_string(void *vstr) __attribute__((hot));
hash_t hashtable_hashfunc_string_sse42(void *vstr) __attribute__((hot));
void hashtable_copyfunc_string(void **dst, void *src);
#define hashtable_freefunc_string free
Hashtable* hashtable_new_stringkeys(size_t size);

void* hashtable_get_string(Hashtable *ht, const char *key);
void hashtable_set_string(Hashtable *ht, const char *key, void *data);
void hashtable_unset_string(Hashtable *ht, const char *key);
void hashtable_unset_deferred_string(Hashtable *ht, const char *key);

void* hashtable_iter_free_data(void *key, void *data, void *arg);
bool hashtable_cmpfunc_ptr(void *p1, void *p2);
void hashtable_copyfunc_ptr(void **dst, void *src);

int hashtable_test(void);

void hashtable_print_stringkeys(Hashtable *ht);
size_t hashtable_get_approx_overhead(Hashtable *ht);
void hashtable_get_stats(Hashtable *ht, HashtableStats *stats);

void hashtable_lock(Hashtable *ht);
void hashtable_unlock(Hashtable *ht);
