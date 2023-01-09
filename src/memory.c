/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "memory.h"

#include <stdlib.h>

#define MEMALIGN_METHOD_C11   0
#define MEMALIGN_METHOD_POSIX 1
#define MEMALIGN_METHOD_WIN32 2

#if defined(TAISEI_BUILDCONF_HAVE_POSIX_MEMALIGN)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_POSIX
#elif defined(TAISEI_BUILDCONF_HAVE_ALIGNED_ALLOC)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_C11
#elif defined(TAISEI_BUILDCONF_HAVE_ALIGNED_MALLOC_FREE)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_WIN32
#else
	#error No usable aligned malloc implementation
#endif

void mem_free(void *ptr) {
#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	_aligned_free(ptr);
#else
	libc_free(ptr);
#endif
}

static size_t mem_calc_array_size(size_t num_members, size_t size) {
	size_t array_size;

	if(__builtin_mul_overflow(num_members, size, &array_size)) {
		assert(0 && "size_t overflow");
		abort();
	}

	return array_size;
}

void *mem_alloc(size_t size) {
#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	void *p = NOT_NULL(_aligned_malloc(size, alignof(max_align_t)));
	memset(p, 0, size);
	return p;
#else
	return NOT_NULL(libc_calloc(1, size));
#endif
}

void *mem_alloc_array(size_t num_members, size_t size) {
#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	size_t array_size = mem_calc_array_size(num_members, size);
	void *p = NOT_NULL(_aligned_malloc(array_size, alignof(max_align_t)));
	memset(p, 0, array_size);
	return p;
#else
	return NOT_NULL(libc_calloc(num_members, size));
#endif
}

void *mem_realloc(void *ptr, size_t size) {
	if(ptr == NULL) {
		return mem_alloc(size);
	}

	if(size == 0) {
		libc_free(ptr);
		return ptr;
	}

#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	return NOT_NULL(_aligned_realloc(ptr, size, alignof(max_align_t)));
#else
	return NOT_NULL(libc_realloc(ptr, size));
#endif
}

void *mem_alloc_aligned(size_t size, size_t alignment) {
	assert((alignment & (alignment - 1)) == 0);
	assert((alignment / sizeof(void*)) * sizeof(void*) == alignment);

#if MEMALIGN_METHOD == MEMALIGN_METHOD_C11
	size_t nsize = ((size - 1) / alignment + 1) * alignment;
	assert(nsize >= size);
	void *p = NOT_NULL(aligned_alloc(alignment, nsize));
	memset(p, 0, size);
	return p;
#elif MEMALIGN_METHOD == MEMALIGN_METHOD_POSIX
	void *p;
	attr_unused int r = posix_memalign(&p, alignment, size);
	assert(r == 0);
	assume(p != NULL);
	memset(p, 0, size);
	return p;
#elif MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	void *p = NOT_NULL(_aligned_malloc(size, alignment));
	memset(p, 0, size);
	return p;
#else
	#error No usable aligned malloc implementation
#endif
}

void *mem_alloc_array_aligned(size_t num_members, size_t size, size_t alignment) {
	return mem_alloc_aligned(mem_calc_array_size(num_members, size), alignment);
}

void *mem_dup(const void *src, size_t size) {
	return memcpy(mem_alloc(size), src, size);
}

// SDL's calling convention may differ from the default, so wrap when necessary.

static void SDLCALL mem_sdlcall_free(void *ptr) {
	mem_free(ptr);
}

static void *SDLCALL mem_sdlcall_malloc(size_t size) {
	return mem_alloc(size);
}

static void *SDLCALL mem_sdlcall_calloc(size_t num_members, size_t size) {
	return mem_alloc_array(num_members, size);
}

static void *SDLCALL mem_sdlcall_realloc(void *ptr, size_t size) {
	return mem_realloc(ptr, size);
}

#define _mem_choose_compatible_func(orig, wrapper) \
	__builtin_choose_expr( \
		__builtin_types_compatible_p(typeof(orig), typeof(wrapper)), \
		orig, wrapper)

void mem_install_sdl_callbacks(void) {
	SDL_SetMemoryFunctions(
		_mem_choose_compatible_func(mem_alloc,       mem_sdlcall_malloc),
		_mem_choose_compatible_func(mem_alloc_array, mem_sdlcall_calloc),
		_mem_choose_compatible_func(mem_realloc,     mem_sdlcall_realloc),
		_mem_choose_compatible_func(mem_free,        mem_sdlcall_free)
	);
}
