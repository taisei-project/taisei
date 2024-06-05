/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "arena.h"

typedef struct MemPool MemPool;

typedef struct MemPoolObjectHeader {
	alignas(alignof(max_align_t)) struct MemPoolObjectHeader *next;
} MemPoolObjectHeader;

struct MemPool {
	MemArena *arena;
	MemPoolObjectHeader *free_objects;
	const char *tag;
	size_t obj_size;
	size_t obj_align;
	int num_allocated;
	int num_used;
};

void mempool_init(
	MemPool *pool,
	const char *tag,
	MemArena *arena,
	size_t obj_size,
	size_t obj_align
) attr_nonnull(1, 2, 3);

void *mempool_acquire(MemPool *pool)
	attr_returns_allocated attr_hot attr_nonnull(1);

void mempool_release(MemPool *pool, void *object)
	attr_hot attr_nonnull(1, 2);
