/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "scratch.h"

#include "log.h"

#include <SDL3/SDL_thread.h>

#define INIT_SCRATCH_SIZE (1 << 12)

typedef struct ScratchArena ScratchArena;

struct ScratchArena {
	MemArena arena;
	ScratchArena *next;
	SDL_ThreadID thread_id;
} ;

typedef struct ScratchTLS {
	MemArena meta_arena;
	ScratchArena *free_arenas;
} ScratchTLS;

static SDL_TLSID tls_id;

static void destroy_tls(void *p) {
	ScratchTLS *tls = p;

	for(auto a = tls->free_arenas; a; a = a->next) {
		marena_deinit(&a->arena);
	}

	marena_deinit(&tls->meta_arena);
	mem_free(tls);
}

static ScratchTLS *get_tls(void) {
	ScratchTLS *tls = SDL_GetTLS(&tls_id);

	if(!tls) {
		tls = ALLOC(ScratchTLS);
		marena_init(&tls->meta_arena, sizeof(ScratchArena) * 4);

		if(!SDL_SetTLS(&tls_id, tls, destroy_tls)) {
			log_sdl_error(LOG_FATAL, "SDL_SetTLS");
		}
	}

	return tls;
}

MemArena *acquire_scratch_arena(void) {
	auto tls = get_tls();

	if(tls->free_arenas) {
		auto scratch = tls->free_arenas;
		tls->free_arenas = scratch->next;
		scratch->next = NULL;
		return &scratch->arena;
	}

	auto scratch = ARENA_ALLOC(&tls->meta_arena, ScratchArena, {});
	scratch->thread_id = SDL_GetCurrentThreadID();
	marena_init(&scratch->arena, INIT_SCRATCH_SIZE);

	return &scratch->arena;
}

void release_scratch_arena(MemArena *arena) {
	auto tls = get_tls();
	auto scratch = UNION_CAST(MemArena*, ScratchArena*, arena);
	assert(scratch->thread_id == SDL_GetCurrentThreadID());
	assert(scratch->next == NULL);

	scratch->next = tls->free_arenas;
	tls->free_arenas = scratch;

	marena_reset(arena);
}
