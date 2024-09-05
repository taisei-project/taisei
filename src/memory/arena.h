/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "list.h"

typedef struct MemArena MemArena;
typedef struct MemArenaPage MemArenaPage;

struct MemArena {
	LIST_ANCHOR(MemArenaPage) pages;
	size_t page_offset;
	size_t total_used;
	size_t total_allocated;
};

struct MemArenaPage {
	LIST_INTERFACE(MemArenaPage);
	MemArena *arena;
	size_t size;
	alignas(alignof(max_align_t)) char data[];
};

void marena_init(MemArena *arena, size_t min_size)
	attr_nonnull_all;

void marena_deinit(MemArena *arena)
	attr_nonnull_all;

void marena_reset(MemArena *arena)
	attr_nonnull_all;

void *marena_alloc(MemArena *arena, size_t size)
	attr_alloc_size(2)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *marena_alloc_array(MemArena *arena, size_t num_members, size_t size)
	attr_alloc_size(2, 3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *marena_alloc_array_aligned(MemArena *arena, size_t num_members, size_t size, size_t align)
	attr_alloc_size(2, 3)
	attr_alloc_align(4)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

void *marena_alloc_aligned(MemArena *arena, size_t size, size_t alignment)
	attr_alloc_size(2)
	attr_alloc_align(3)
	attr_malloc
	attr_returns_allocated
	attr_nonnull_all;

bool marena_free(MemArena *restrict arena, void *restrict p, size_t old_size);

#define marena_assert_free(arena, p, old_size) ({ \
	bool _freed = marena_free(arena, p, old_size); \
	assert(_freed); \
	(void)0; \
})

void *marena_realloc(MemArena *restrict arena, void *restrict p, size_t old_size, size_t new_size)
	attr_alloc_size(4)
	attr_nonnull(1)
	attr_returns_allocated;

void *marena_realloc_aligned(MemArena *restrict arena, void *restrict p, size_t old_size, size_t new_size, size_t align)
	attr_alloc_size(4)
	attr_alloc_align(5)
	attr_nonnull(1)
	attr_returns_allocated;

INLINE void *marena_memdup(MemArena *arena, const void *buf, size_t size) {
	return memcpy(marena_alloc(arena, size), buf, size);
}

INLINE char *marena_strdup(MemArena *arena, const char *src) {
	return marena_memdup(arena, src, strlen(src) + 1);
}

/*
 * These are similar to the macros in memory.h, except they also take an Arena pointer as the
 * first argument.
 */

#define ARENA_ALLOC(_arena, _type, ...)\
	MACROHAX_OVERLOAD_HASARGS(ARENA_ALLOC_, __VA_ARGS__)(_arena, _type, ##__VA_ARGS__)

#define ARENA_ALLOC_0(_arena, _type) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		marena_alloc_aligned(_arena, sizeof(_type), alignof(_type)), \
		marena_alloc(_arena, sizeof(_type)))

#define ARENA_ALLOC_1(_arena, _type, ...) ({ \
		auto _alloc_ptr = ARENA_ALLOC_0(_arena, _type); \
		*_alloc_ptr = (_type) __VA_ARGS__; \
		_alloc_ptr; \
	})

#define ARENA_ALLOC_ARRAY(_arena, _nmemb, _type) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		marena_alloc_array_aligned(_arena, _nmemb, sizeof(_type), alignof(_type)), \
		marena_alloc_array(_arena, _nmemb, sizeof(_type)))

#define ARENA_ALLOC_FLEX(_arena, _type, _extra_size) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		marena_alloc_aligned(_arena, sizeof(_type) + (_extra_size), alignof(_type)), \
		marena_alloc(_arena, sizeof(_type) + (_extra_size)))
