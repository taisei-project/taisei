/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdlib.h>
#include <stdbool.h>

#include "list.h"

#ifdef DEBUG
    #define OBJPOOL_DEBUG
#endif

#ifdef OBJPOOL_DEBUG
    #define IF_OBJPOOL_DEBUG(code) code
#else
    #define IF_OBJPOOL_DEBUG(code)
#endif

typedef struct ObjectPool ObjectPool;
typedef struct ObjectInterface ObjectInterface;
typedef struct ObjectPoolStats ObjectPoolStats;

struct ObjectPoolStats {
    const char *tag;
    size_t capacity;
    size_t usage;
    size_t peak_usage;
};

struct ObjectInterface {
    union {
        List list_interface;
        struct {
            ObjectInterface *next;
            ObjectInterface *prev;
        };
    };
};

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag);
void objpool_free(ObjectPool *pool);
ObjectInterface *objpool_acquire(ObjectPool *pool);
void objpool_release(ObjectPool *pool, ObjectInterface *object);
void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats);
