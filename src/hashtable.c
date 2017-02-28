/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "hashtable.h"
#include "list.h"

#include <assert.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>

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
};

/*
 *  Generic functions
 */

Hashtable* hashtable_new(size_t size, HTCmpFunc cmp_func, HTHashFunc hash_func, HTCopyFunc copy_func, HTFreeFunc free_func) {
    assert(size > 0);

    Hashtable *ht = malloc(sizeof(Hashtable));
    ht->table = calloc(size, sizeof(HashtableElement*));
    ht->table_size = size;
    ht->cmp_func = cmp_func;
    ht->hash_func = hash_func;
    ht->copy_func = copy_func;
    ht->free_func = free_func;

    return ht;
}

static void hashtable_delete_callback(void **vlist, void *velem, void *vht) {
    Hashtable *ht = vht;
    HashtableElement *elem = velem;

    if(ht->free_func) {
        ht->free_func(elem->key);
    }

    delete_element(vlist, velem);
}

void hashtable_unset_all(Hashtable *ht) {
    for(size_t i = 0; i < ht->table_size; ++i) {
        delete_all_elements_witharg((void**)(ht->table + i), hashtable_delete_callback, ht);
    }
}

void hashtable_free(Hashtable *ht) {
    if(!ht) {
        return;
    }

    hashtable_unset_all(ht);
    free(ht->table);
    free(ht);
}

static inline bool hashtable_compare(Hashtable *ht, void *key1, void *key2) {
    if(ht->cmp_func) {
        return ht->cmp_func(key1, key2);
    }

    return (key1 == key2);
}

void* hashtable_get(Hashtable *ht, void *key) {
    hash_t hash = ht->hash_func(key);
    HashtableElement *elems = ht->table[hash % ht->table_size];

    for(HashtableElement *e = elems; e; e = e->next) {
        if(hash == e->hash && hashtable_compare(ht, key, e->key)) {
            return e->data;
        }
    }

    return NULL;
}

void hashtable_set(Hashtable *ht, void *key, void *data) {
    hash_t hash = ht->hash_func(key);
    size_t idx = hash % ht->table_size;
    HashtableElement *elems = ht->table[idx], *elem;

    for(HashtableElement *e = elems; e; e = e->next) {
        if(hash == e->hash && hashtable_compare(ht, key, e->key)) {
            if(ht->free_func) {
                ht->free_func(e->key);
            }
            delete_element((void**)&elems, e);
        }
    }

    if(data) {
        elem = create_element((void**)&elems, sizeof(HashtableElement));

        if(ht->copy_func) {
            ht->copy_func(&elem->key, key);
        } else {
            elem->key = key;
        }

        elem->hash = ht->hash_func(elem->key);
        elem->data = data;
    }

    ht->table[idx] = elems;
}

void hashtable_unset(Hashtable *ht, void *key) {
    hashtable_set(ht, key, NULL);
}

void* hashtable_foreach(Hashtable *ht, HTIterCallback callback, void *arg) {
    void *ret = NULL;

    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            ret = callback(e->key, e->data, arg);
            if(ret) {
                return ret;
            }
        }
    }

    return ret;
}

/*
 *  Convenience functions for hashtables with string keys
 */

bool hashtable_cmpfunc_string(void *str1, void *str2) {
    return !strcmp((const char*)str1, (const char*)(str2));
}

hash_t hashtable_hashfunc_string(void *vstr) {
    const char *str = vstr;
    size_t len = strlen(str);
    return adler32_z(0, (const Bytef*)str, len);
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

void hashtable_print_stringkeys(Hashtable *ht) {
    printf("------ %p:\n", (void*)ht);
    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            printf("[HT %lu] %s (%lu): %p\n", (unsigned long)i, (char*)e->key, e->hash, e->data);
        }
    }
}

/*
 *  Misc convenience functions
 */

// #define HASHTABLE_TEST

#ifdef HASHTABLE_TEST

#include <stdio.h>

static void hashtable_printstrings(Hashtable *ht) {
    for(size_t i = 0; i < ht->table_size; ++i) {
        for(HashtableElement *e = ht->table[i]; e; e = e->next) {
            printf("[HT %lu] %s (%lu): %s\n", (unsigned long)i, (char*)e->key, e->hash, (char*)e->data);
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
    printf("-----\n");
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
