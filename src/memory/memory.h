/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/macrohax.h"

#include <SDL3/SDL.h>

/*
 * NOTE: All allocation functions return zero-initialized memory (except realloc) and never NULL.
 */

void mem_free(void *ptr);

void *mem_alloc(size_t size)
	attr_malloc
	attr_dealloc(mem_free, 1)
	attr_alloc_size(1)
	attr_returns_allocated;

void *mem_alloc_array(size_t num_members, size_t size)
	attr_malloc
	attr_dealloc(mem_free, 1)
	attr_alloc_size(1, 2)
	attr_returns_allocated;

void *mem_realloc(void *ptr, size_t size)
	attr_dealloc(mem_free, 1)
	attr_alloc_size(2)
	attr_returns_allocated;

void *mem_alloc_aligned(size_t size, size_t alignment)
	attr_malloc
	attr_dealloc(mem_free, 1)
	attr_alloc_size(1)
	attr_alloc_align(2)
	attr_returns_allocated;

void *mem_alloc_array_aligned(size_t num_members, size_t size, size_t alignment)
	attr_malloc
	attr_dealloc(mem_free, 1)
	attr_alloc_size(1, 2)
	attr_alloc_align(3)
	attr_returns_allocated;

void *mem_dup(const void *src, size_t size)
	attr_malloc
	attr_dealloc(mem_free, 1)
	attr_alloc_size(2)
	attr_returns_allocated;

#define memdup mem_dup

INLINE attr_returns_allocated attr_nonnull(1)
char *mem_strdup(const char *str) {
	size_t sz = strlen(str) + 1;
	return memcpy(mem_alloc(sz), str, sz);
}

/*
 * This macro transparently handles allocation of both normal and over-aligned types.
 * The allocation must be free'd with mem_free().
 *
 * You should use it like this:
 *
 * 		auto x = ALLOC(MyType);  // x is a pointer to MyType
 *
 * This style is also allowed:
 *
 * 		MyType *x = ALLOC(typeof(*x));
 *
 * This style can also be used, but is not recommended:
 *
 * 		MyType *x = ALLOC(MyType);
 *
 * You can also provide an optional initializer as the second argument, like so:
 *
 * 		auto x = ALLOC(MyStructType, { .foo = 42, .bar = 69 });
 */
#define ALLOC(_type, ...)\
	MACROHAX_OVERLOAD_HASARGS(ALLOC_, __VA_ARGS__)(_type, ##__VA_ARGS__)

#define ALLOC_0(_type) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		mem_alloc_aligned(sizeof(_type), alignof(_type)), \
		mem_alloc(sizeof(_type)))

#define ALLOC_1(_type, ...) ({ \
		auto _alloc_ptr = ALLOC_0(_type); \
		*_alloc_ptr = (_type) __VA_ARGS__; \
		_alloc_ptr; \
	})

/*
 * Like ALLOC(), but for array allocations:
 *
 * 		auto x = ALLOC_ARRAY(10, int);  // x is an int*
 *
 * Unlike ALLOC(), this macro does not accept an optional initializer.
 */
#define ALLOC_ARRAY(_nmemb, _type) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		mem_alloc_array_aligned(_nmemb, sizeof(_type), alignof(_type)), \
		mem_alloc_array(_nmemb, sizeof(_type)))

/*
 * Like ALLOC(), but for structs with flexible array members:
 *
 * 		auto x = ALLOC(MyFlexStruct, 42);  // allocates sizeof(MyFlexStruct)+42 bytes
 *
 * Unlike ALLOC(), this macro does not accept an optional initializer.
 */
#define ALLOC_FLEX(_type, _extra_size) \
	(_type *)__builtin_choose_expr( \
		alignof(_type) > alignof(max_align_t), \
		mem_alloc_aligned(sizeof(_type) + (_extra_size), alignof(_type)), \
		mem_alloc(sizeof(_type) + (_extra_size)))

void mem_install_sdl_callbacks(void);
