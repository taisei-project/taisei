/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "memory.h"
#include "util.h"

#include "../util.h"

#define MEMALIGN_METHOD_C11   0
#define MEMALIGN_METHOD_POSIX 1
#define MEMALIGN_METHOD_WIN32 2
#define MEMALIGN_METHOD_YOLO  3

#if defined(TAISEI_BUILDCONF_HAVE_POSIX_MEMALIGN)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_POSIX
#elif defined(TAISEI_BUILDCONF_HAVE_ALIGNED_ALLOC)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_C11
#elif defined(TAISEI_BUILDCONF_HAVE_ALIGNED_MALLOC_FREE)
	#define MEMALIGN_METHOD MEMALIGN_METHOD_WIN32
#else
	#error No usable aligned malloc implementation
#endif

#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	#warning "Not using win32 _aligned_alloc because it's known to crash. Over-aligned allocations will NOT be supported! Please use a custom allocator like mimalloc."
	#undef MEMALIGN_METHOD
	#define MEMALIGN_METHOD MEMALIGN_METHOD_YOLO
#endif

void mem_free(void *ptr) {
#if MEMALIGN_METHOD == MEMALIGN_METHOD_WIN32
	_aligned_free(ptr);
#else
	libc_free(ptr);
#endif
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
	size_t array_size = mem_util_calc_array_size(num_members, size);
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

	assert_nolog(size > 0);

	if(size == 0) {
		mem_free(ptr);
		DIAGNOSTIC(push)
		DIAGNOSTIC(ignored "-Wnonnull")
		return NULL;
		DIAGNOSTIC(pop)
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
#elif MEMALIGN_METHOD == MEMALIGN_METHOD_YOLO
	return mem_alloc(size);
#else
	#error No usable aligned malloc implementation
#endif
}
