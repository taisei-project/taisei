/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "arena.h"

#include "util.h"
#include "util/miscmath.h"
#include "../util.h"

#define ARENA_MIN_ALLOC 4096

// NOTE: if we cared about 64-bit Linux only, mmap would be nice here…
// Unfortunately, some of the platforms we support *cough*emscripten*cough*
// don't even have virtual memory, so we can't have nice infinitely growable
// contiguous arenas.

static void *_arena_alloc_page(size_t s) {
	return mem_alloc(s);
}

static void _arena_dealloc_page(MemArenaPage *p) {
	mem_free(p);
}

static MemArenaPage *_arena_new_page(MemArena *arena, size_t min_size) {
	auto alloc_size = topow2_u64(min_size + sizeof(MemArenaPage));
	alloc_size = max(min_size, ARENA_MIN_ALLOC);
	auto page_size = alloc_size - sizeof(MemArenaPage);
	MemArenaPage *p = _arena_alloc_page(alloc_size);
	p->size = page_size;
	p->arena = arena;
	alist_append(&arena->pages, p);
	arena->page_offset = 0;
	arena->total_allocated += page_size;
	return p;
}

static void _arena_delete_page(MemArena *arena, MemArenaPage *page) {
	assume(page->arena == arena);
	_arena_dealloc_page(page);
}

static void *_arena_alloc(MemArena *arena, size_t size, size_t align) {
	auto page = NOT_NULL(arena->pages.last);
	assume(page->arena == arena);
	assume(page->next == NULL);
	auto page_ofs = arena->page_offset;
	size_t required, alignofs;

	for(;;) {
		auto available = page->size - page_ofs;
		alignofs = align - ((uintptr_t)(page->data + page_ofs) & (align - 1));
		required = alignofs + size;

		if(available < required) {
			page = _arena_new_page(arena, required);
			assert(arena->page_offset == 0);
			page_ofs = 0;
			continue;
		}

		break;
	}

	void *p = page->data + page_ofs + alignofs;
	arena->total_used += required;
	arena->page_offset += required;
	assert(arena->page_offset <= page->size);
	assert(((uintptr_t)p & (align - 1)) == 0);
	return p;
}

void marena_init(MemArena *arena, size_t min_size) {
	*arena = (MemArena) { };
	_arena_new_page(arena, min_size);
}

void marena_deinit(MemArena *arena) {
	MemArenaPage *p;
	while((p = alist_pop(&arena->pages))) {
		_arena_delete_page(arena, p);
	}
}

void marena_reset(MemArena *arena) {
	auto used = arena->total_used;
	arena->total_used = 0;
	arena->page_offset = 0;

	if(arena->pages.first->next) {
		MemArenaPage *p;
		while((p = alist_pop(&arena->pages))) {
			_arena_delete_page(arena, p);
		}

		arena->total_allocated = 0;
		p = _arena_new_page(arena, used);
		assert(p == arena->pages.last);
	}

	assert(arena->pages.first != NULL);
	assert(arena->pages.first == arena->pages.last);
}

void *marena_alloc(MemArena *arena, size_t size) {
	return _arena_alloc(arena, size, alignof(max_align_t));
}

void *marena_alloc_array(MemArena *arena, size_t num_members, size_t size) {
	return _arena_alloc(arena, mem_util_calc_array_size(num_members, size), alignof(max_align_t));
}

void *marena_alloc_aligned(MemArena *arena, size_t size, size_t align) {
	assume(align > 0);
	assume(!(align & (align - 1)));

	if(align < alignof(max_align_t)) {
		align = alignof(max_align_t);
	}

	return _arena_alloc(arena, size, align);
}
