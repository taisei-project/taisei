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
	void *data;
	void *key;
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
		cmp_func = hashtable_cmpfunc_ptr;
	}

	if(!copy_func) {
		copy_func = hashtable_copyfunc_ptr;
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

	assert(ht->hash_func != NULL);

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

void* hashtable_get(Hashtable *ht, void *key) {
	assert(ht != NULL);

	hash_t hash = ht->hash_func(key);
	hashtable_begin_read(ht);
	HashtableElement *elems = ht->table[hash & ht->hash_mask];

	for(HashtableElement *e = elems; e; e = e->next) {
		if(hash == e->hash && ht->cmp_func(key, e->key)) {
			hashtable_end_read(ht);
			return e->data;
		}
	}

	hashtable_end_read(ht);
	return NULL;
}

void* hashtable_get_unsafe(Hashtable *ht, void *key) {
	hash_t hash = ht->hash_func(key);
	HashtableElement *elems = ht->table[hash & ht->hash_mask];

	for(HashtableElement *e = elems; e; e = e->next) {
		if(hash == e->hash && ht->cmp_func(key, e->key)) {
			return e->data;
		}
	}

	return NULL;
}

static void hashtable_set_internal(Hashtable *ht, HashtableElement **table, size_t hash_mask, hash_t hash, void *key, void *data) {
	size_t idx = hash & hash_mask;
	HashtableElement *elems = table[idx], *elem;

	for(HashtableElement *e = elems; e; e = e->next) {
		if(hash == e->hash && ht->cmp_func(key, e->key)) {
			if(ht->free_func) {
				ht->free_func(e->key);
			}

			free(list_unlink(&elems, e));
			ht->num_elements--;
			break;
		}
	}

	if(data) {
		elem = malloc(sizeof(HashtableElement));
		ht->copy_func(&elem->key, key);
		elem->hash = ht->hash_func(elem->key);
		elem->data = data;
		list_push(&elems, elem);
		ht->num_elements++;
	}

	table[idx] = elems;
}

static void hashtable_resize(Hashtable *ht, size_t new_size) {
	assert(new_size != ht->table_size);
	HashtableElement **new_table = calloc(new_size, sizeof(HashtableElement*));
	size_t new_hash_mask = new_size - 1;

	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HashtableElement *e = ht->table[i]; e; e = e->next) {
			hashtable_set_internal(ht, new_table, new_hash_mask, e->hash, e->key, e->data);
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

void hashtable_set(Hashtable *ht, void *key, void *data) {
	assert(ht != NULL);

	hash_t hash = ht->hash_func(key);

	hashtable_begin_write(ht);
	hashtable_set_internal(ht, ht->table, ht->hash_mask, hash, key, data);

	if(ht->num_elements == ht->table_size) {
		hashtable_resize(ht, ht->table_size * 2);
	}

	hashtable_end_write(ht);
}

void hashtable_unset(Hashtable *ht, void *key) {
	hashtable_set(ht, key, NULL);
}

void hashtable_unset_deferred(Hashtable *ht, void *key, ListContainer **list) {
	assert(ht != NULL);
	ListContainer *c = list_push(list, list_wrap_container(NULL));
	ht->copy_func(&c->data, key);
}

void hashtable_unset_deferred_now(Hashtable *ht, ListContainer **list) {
	ListContainer *next;
	assert(ht != NULL);

	for(ListContainer *c = *list; c; c = next) {
		next = c->next;
		hashtable_unset(ht, c->data);

		if(ht->free_func) {
			ht->free_func(c->data);
		}

		free(list_unlink(list, c));
	}
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

hash_t hashtable_hashfunc_string_sse42(void *vstr) {
	return crc32str_sse42(0, (const char*)vstr);
}

void hashtable_copyfunc_string(void **dst, void *src) {
	*dst = malloc(strlen((char*)src) + 1);
	strcpy(*dst, src);
}

// #define hashtable_freefunc_string free

Hashtable* hashtable_new_stringkeys(void) {
	return hashtable_new(
		hashtable_cmpfunc_string,
		SDL_HasSSE42() ? hashtable_hashfunc_string_sse42 : hashtable_hashfunc_string,
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

hash_t hashtable_hashfunc_ptr(void *val) {
	hash_t x = (uintptr_t)val & ((1u << 31u) - 1u);
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
			log_debug(" -- %s (%"PRIuMAX"): %p", (char*)e->key, (uintmax_t)e->hash, e->data);
		}
	}

	log_debug(
		"%i total elements, %i unused buckets, %i collisions, max %i elems per bucket, %lu approx overhead",
		stats.num_elements, stats.free_buckets, stats.collisions, stats.max_per_bucket,
		(ulong)hashtable_get_approx_overhead(ht)
	);
}
