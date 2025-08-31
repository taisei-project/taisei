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
#include "util/miscmath.h"

#include <mimalloc.h>

#define MIN_ALIGNMENT alignof(max_align_t)

void mem_free(void *ptr) {
	mi_free(ptr);
}

void *mem_alloc(size_t size) {
	return mem_alloc_aligned(size, MIN_ALIGNMENT);
}

void *mem_alloc_array(size_t num_members, size_t size) {
	size_t alloc_size = mem_util_calc_array_size(num_members, size);
	return mem_alloc_aligned(alloc_size, MIN_ALIGNMENT);
}

void *mem_realloc(void *ptr, size_t size) {
	assert_nolog(size > 0);
	return mi_realloc_aligned(ptr, size, MIN_ALIGNMENT);
}

void *mem_alloc_aligned(size_t size, size_t alignment) {
	alignment = max(MIN_ALIGNMENT, alignment);
	return mi_calloc_aligned(1, size, alignment);
}
