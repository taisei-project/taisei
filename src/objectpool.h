/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "list.h"
#include "memory/arena.h"

typedef struct ObjectPool ObjectPool;
typedef struct ObjectPoolStats ObjectPoolStats;

typedef struct ObjHeader {
	alignas(alignof(max_align_t)) struct ObjHeader *next;
} ObjHeader;

struct ObjectPool {
	MemArena *arena;
	ObjHeader *free_objects;
	const char *tag;
	size_t obj_size;
	size_t obj_align;
	int num_allocated;
	int num_used;
};

void objpool_init(
	ObjectPool *pool,
	const char *tag,
	MemArena *arena,
	size_t obj_size,
	size_t obj_align
) attr_nonnull(1, 2, 3);

void *objpool_acquire(ObjectPool *pool)
	attr_returns_allocated attr_hot attr_nonnull(1);

void objpool_release(ObjectPool *pool, void *object)
	attr_hot attr_nonnull(1, 2);
