/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "memory.h"

#include "util.h"

#include <stdlib.h>

void *mem_alloc_array_aligned(size_t num_members, size_t size, size_t alignment) {
	return mem_alloc_aligned(mem_util_calc_array_size(num_members, size), alignment);
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
