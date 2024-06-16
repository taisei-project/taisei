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

typedef struct MemPoolObjectHeader {
	alignas(alignof(max_align_t)) struct MemPoolObjectHeader *next;
} MemPoolObjectHeader;

#define MEMPOOL_BASE(elem_type) struct { \
	union { \
		MemPoolObjectHeader *as_generic; \
		elem_type *as_specific; \
	} free_objects; \
	uint num_allocated; \
	uint num_used; \
} \

typedef MEMPOOL_BASE(max_align_t) MemPool;

#define MEMPOOL(elem_type) union { \
	MEMPOOL_BASE(elem_type); \
	MemPool as_generic; \
}

#define MEMPOOL_ASSERT_VALID(mpool) do { \
	static_assert( \
		__builtin_types_compatible_p(MemPool, __typeof__((mpool)->as_generic)), \
		"x->as_generic must be of MemPool type"); \
	static_assert( \
		__builtin_offsetof(__typeof__(*(mpool)), as_generic) == 0, \
		"x->as_generic must be the first member in struct"); \
} while(0)

#define MEMPOOL_CAST_TO_BASE(mpool) ({ \
	MEMPOOL_ASSERT_VALID(mpool); \
	&NOT_NULL(mpool)->as_generic; \
})

#define MEMPOOL_OBJTYPE(mpool) \
	typeof(*(mpool)->free_objects.as_specific)

void *mempool_generic_acquire(MemPool *pool, MemArena *arena, size_t size, size_t align)
	attr_returns_allocated attr_hot attr_nonnull(1, 2);

void mempool_generic_release(MemPool *pool, void *object)
	attr_hot attr_nonnull(1, 2);

#define mempool_acquire(mpool, arena) ({ \
	auto _mpool = mpool; \
	MEMPOOL_OBJTYPE(_mpool) *_obj = mempool_generic_acquire(\
		MEMPOOL_CAST_TO_BASE(_mpool), \
		(arena), \
		sizeof(MEMPOOL_OBJTYPE(_mpool)), \
		alignof(MEMPOOL_OBJTYPE(_mpool))); \
	*_obj = (typeof(*_obj)) {} ; \
	_obj; \
})

#define mempool_release(mpool, obj) ({ \
	static_assert( \
		__builtin_types_compatible_p(typeof(*(obj)), MEMPOOL_OBJTYPE(mpool))); \
	mempool_generic_release(MEMPOOL_CAST_TO_BASE(mpool), (obj)); \
})
