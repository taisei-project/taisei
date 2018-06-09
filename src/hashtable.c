/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "hashtable.h"
#include "list.h"
#include "util.h"

#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <SDL_mutex.h>

// #define HT_USE_MUTEX

typedef struct HashtableElement {
	LIST_INTERFACE(struct HashtableElement);
	HashtableValue data;
	HashtableValue key;
	hash_t hash;
} HashtableElement;

struct Hashtable {
	HashtableElement **table;
	HTCmpFunc cmp_func;
	HTHashFunc hash_func;
	HTCopyFunc copy_func;
	HTFreeFunc free_func;
	size_t num_elements;
	size_t table_size;
	size_t hash_mask;

	struct {
		SDL_mutex *mutex;
		SDL_cond *cond;
		uint readers;
		bool writing;
	} sync;
};

typedef struct HashtableIterator {
	Hashtable *hashtable;
	size_t bucketnum;
	HashtableElement *elem;
} HashtableIterator;

/*
 *  Generic functions
 */

Hashtable* hashtable_new(HTCmpFunc cmp_func, HTHashFunc hash_func, HTCopyFunc copy_func, HTFreeFunc free_func) {
	Hashtable *ht = malloc(sizeof(Hashtable));
	size_t size = HT_MIN_SIZE;

	if(!cmp_func) {
		cmp_func = hashtable_cmpfunc_raw;
	}

	if(!copy_func) {
		copy_func = hashtable_copyfunc_raw;
	}

	ht->table = calloc(size, sizeof(HashtableElement*));
	ht->table_size = size;
	ht->hash_mask = size - 1;
	ht->num_elements = 0;
	ht->cmp_func = cmp_func;
	ht->hash_func = hash_func;
	ht->copy_func = copy_func;
	ht->free_func = free_func;

	ht->sync.writing = false;
	ht->sync.readers = 0;
	ht->sync.mutex = SDL_CreateMutex();
	ht->sync.cond = SDL_CreateCond();

	return ht;
}

static void hashtable_begin_write(Hashtable *ht) {
	SDL_LockMutex(ht->sync.mutex);

	while(ht->sync.writing || ht->sync.readers) {
		SDL_CondWait(ht->sync.cond, ht->sync.mutex);
	}

	ht->sync.writing = true;
	SDL_UnlockMutex(ht->sync.mutex);
}

static void hashtable_end_write(Hashtable *ht) {
	SDL_LockMutex(ht->sync.mutex);
	ht->sync.writing = false;
	SDL_CondBroadcast(ht->sync.cond);
	SDL_UnlockMutex(ht->sync.mutex);
}

static void hashtable_begin_read(Hashtable *ht) {
	SDL_LockMutex(ht->sync.mutex);

	while(ht->sync.writing) {
		SDL_CondWait(ht->sync.cond, ht->sync.mutex);
	}

	++ht->sync.readers;
	SDL_UnlockMutex(ht->sync.mutex);
}

static void hashtable_end_read(Hashtable *ht) {
	SDL_LockMutex(ht->sync.mutex);

	if(!--ht->sync.readers) {
		SDL_CondSignal(ht->sync.cond);
	}

	SDL_UnlockMutex(ht->sync.mutex);
}

void hashtable_lock(Hashtable *ht) {
	assert(ht != NULL);
	hashtable_begin_read(ht);
}

void hashtable_unlock(Hashtable *ht) {
	assert(ht != NULL);
	hashtable_end_read(ht);
}

static void* hashtable_delete_callback(List **vlist, List *velem, void *vht) {
	Hashtable *ht = vht;
	HashtableElement *elem = (HashtableElement*)velem;

	if(ht->free_func) {
		ht->free_func(elem->key);
	}

	free(list_unlink(vlist, velem));
	return NULL;
}

static void hashtable_unset_all_internal(Hashtable *ht) {
	for(size_t i = 0; i < ht->table_size; ++i) {
		list_foreach((ht->table + i), hashtable_delete_callback, ht);
	}

	ht->num_elements = 0;
}

void hashtable_unset_all(Hashtable *ht) {
	assert(ht != NULL);
	hashtable_begin_write(ht);
	hashtable_unset_all_internal(ht);
	hashtable_end_write(ht);
}

void hashtable_free(Hashtable *ht) {
	if(!ht) {
		return;
	}

	hashtable_unset_all(ht);

	SDL_DestroyCond(ht->sync.cond);
	SDL_DestroyMutex(ht->sync.mutex);

	free(ht->table);
	free(ht);
}

static HashtableElement* hashtable_get_internal(Hashtable *ht, HashtableValue key, hash_t hash) {
	HashtableElement *elems = ht->table[hash & ht->hash_mask];

	for(HashtableElement *e = elems; e; e = e->next) {
		if(hash == e->hash && ht->cmp_func(key, e->key)) {
			return e;
		}
	}

	return NULL;
}

HashtableValue _hashtable_get_vunion(Hashtable *ht, HashtableValue key) {
	hash_t hash = ht->hash_func(key);
	HashtableElement *elem;
	HashtableValue data;

	hashtable_begin_read(ht);
	elem = hashtable_get_internal(ht, key, hash);
	data = elem ? elem->data : HT_NULL_VAL;
	hashtable_end_read(ht);

	return data;
}

HashtableValue _hashtable_get_unsafe_vunion(Hashtable *ht, HashtableValue key) {
	hash_t hash = ht->hash_func(key);
	HashtableElement *elem;
	HashtableValue data;

	elem = hashtable_get_internal(ht, key, hash);
	data = elem ? elem->data : HT_NULL_VAL;

	return data;
}

static bool hashtable_set_internal(
	Hashtable *ht,
	HashtableElement **table,
	size_t hash_mask,
	hash_t hash,
	HashtableValue key,
	HashtableValue data,
	HashtableValue (*datafunc)(HashtableValue),
	bool allow_overwrite,
	HashtableValue *val
) {
	size_t idx = hash & hash_mask;
	HashtableElement *elems = table[idx], *elem;
	bool was_set = false;

	for(HashtableElement *e = elems; e; e = e->next) {
		if(hash == e->hash && ht->cmp_func(key, e->key)) {
			if(!allow_overwrite) {
				if(val != NULL) {
					*val = e->data;
				}

				return was_set;
			}

			if(ht->free_func) {
				ht->free_func(e->key);
			}

			free(list_unlink(&elems, e));
			ht->num_elements--;
			break;
		}
	}

	if(datafunc != NULL) {
		data = datafunc(data);
	}

	if(val != NULL) {
		*val = data;
	}

	if(!HT_IS_NULL(data)) {
		elem = malloc(sizeof(HashtableElement));
		ht->copy_func(&elem->key, key);
		elem->hash = ht->hash_func(elem->key);
		elem->data = data;
		list_push(&elems, elem);
		ht->num_elements++;
		was_set = true;
	}

	table[idx] = elems;
	return was_set;
}

static void hashtable_resize(Hashtable *ht, size_t new_size) {
	assert(new_size != ht->table_size);
	HashtableElement **new_table = calloc(new_size, sizeof(HashtableElement*));
	size_t new_hash_mask = new_size - 1;

	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HashtableElement *e = ht->table[i]; e; e = e->next) {
			hashtable_set_internal(ht, new_table, new_hash_mask, e->hash, e->key, e->data, NULL, true, NULL);
		}
	}

	hashtable_unset_all_internal(ht);
	free(ht->table);

	ht->table = new_table;

	log_debug("Resized hashtable at %p: %"PRIuMAX" -> %"PRIuMAX"",
		(void*)ht, (uintmax_t)ht->table_size, (uintmax_t)new_size);
	// hashtable_print_stringkeys(ht);

	ht->table_size = new_size;
	ht->hash_mask = new_hash_mask;
}

void _hashtable_set_vunion_vunion(Hashtable *ht, HashtableValue key, HashtableValue data) {
	hash_t hash = ht->hash_func(key);

	hashtable_begin_write(ht);
	hashtable_set_internal(ht, ht->table, ht->hash_mask, hash, key, data, NULL, true, NULL);

	if(ht->num_elements == ht->table_size) {
		hashtable_resize(ht, ht->table_size * 2);
	}

	hashtable_end_write(ht);
}

bool _hashtable_try_set_vunion_vunion(Hashtable *ht, HashtableValue key, HashtableValue data, HashtableValue (*datafunc)(HashtableValue), HashtableValue *val) {
	hash_t hash = ht->hash_func(key);

	hashtable_begin_write(ht);
	bool result = hashtable_set_internal(ht, ht->table, ht->hash_mask, hash, key, data, datafunc, false, val);

	if(ht->num_elements == ht->table_size) {
		hashtable_resize(ht, ht->table_size * 2);
	}

	hashtable_end_write(ht);
	return result;
}

void _hashtable_unset_vunion(Hashtable *ht, HashtableValue key) {
	hashtable_set(ht, key, NULL);
}

/*
 *  Iteration functions
 */

void* hashtable_foreach(Hashtable *ht, HTIterCallback callback, void *arg) {
	assert(ht != NULL);

	void *ret = NULL;

	hashtable_begin_read(ht);

	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HashtableElement *e = ht->table[i]; e; e = e->next) {
			ret = callback(e->key, e->data, arg);
			if(ret) {
				hashtable_end_read(ht);
				return ret;
			}
		}
	}

	hashtable_end_read(ht);

	return ret;
}

HashtableIterator* hashtable_iter(Hashtable *ht) {
	assert(ht != NULL);
	HashtableIterator *iter = malloc(sizeof(HashtableIterator));
	iter->hashtable = ht;
	iter->bucketnum = (size_t)-1;
	return iter;
}

bool _hashtable_iter_next_vunion_vunion(HashtableIterator *iter, HashtableValue *out_key, HashtableValue *out_data) {
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

bool hashtable_cmpfunc_string(HashtableValue str1, HashtableValue str2) {
	return !strcmp(str1.pointer, str2.pointer);
}

hash_t hashtable_hashfunc_string(HashtableValue vstr) {
	return crc32str(0, vstr.pointer);
}

hash_t hashtable_hashfunc_string_sse42(HashtableValue vstr) {
	return crc32str_sse42(0, vstr.pointer);
}

void hashtable_copyfunc_string(HashtableValue *dst, HashtableValue src) {
	dst->pointer = strdup(src.pointer);
}

void hashtable_freefunc_free(HashtableValue val) {
	free(val.pointer);
}

Hashtable* hashtable_new_stringkeys(void) {
	return hashtable_new(
		hashtable_cmpfunc_string,
		SDL_HasSSE42() ? hashtable_hashfunc_string_sse42 : hashtable_hashfunc_string,
		hashtable_copyfunc_string,
		hashtable_freefunc_string
	);
}

/*
 *  Misc convenience functions
 */

void* hashtable_iter_free_data(HashtableValue key, HashtableValue data, void *arg) {
	free(data.pointer);
	return NULL;
}

bool hashtable_cmpfunc_ptr(HashtableValue p1, HashtableValue p2) {
	return p1.pointer == p2.pointer;
}

void hashtable_copyfunc_ptr(HashtableValue *dst, HashtableValue src) {
	dst->pointer = src.pointer;
}

bool hashtable_cmpfunc_raw(HashtableValue p1, HashtableValue p2) {
	return !memcmp(&p1, &p2, sizeof(p1));
}

void hashtable_copyfunc_raw(HashtableValue *dst, HashtableValue src) {
	memcpy(dst, &src, sizeof(*dst));
}

hash_t hashtable_hashfunc_int(HashtableValue val) {
	hash_t x = val.uint64 & ((1u << 31u) - 1u);
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
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
			log_debug(" -- %s (%"PRIuMAX"): %p", (char*)e->key.pointer, (uintmax_t)e->hash, e->data.pointer);
		}
	}

	log_debug(
		"%i total elements, %i unused buckets, %i collisions, max %i elems per bucket, %lu approx overhead",
		stats.num_elements, stats.free_buckets, stats.collisions, stats.max_per_bucket,
		(ulong)hashtable_get_approx_overhead(ht)
	);
}
