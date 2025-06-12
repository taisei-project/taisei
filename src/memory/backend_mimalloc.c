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

#include <mimalloc.h>

void mem_free(void *ptr) {
	mi_free(ptr);
}

void *mem_alloc(size_t size) {
	return mi_calloc(1, size);
}

void *mem_alloc_array(size_t num_members, size_t size) {
	return mi_calloc(num_members, size);
}

void *mem_realloc(void *ptr, size_t size) {
	assert_nolog(size > 0);
	return mi_realloc(ptr, size);
}

void *mem_alloc_aligned(size_t size, size_t alignment) {
	return mi_calloc_aligned(1, size, alignment);
}
