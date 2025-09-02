/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/arena.h"

#include <SDL3/SDL.h>

typedef struct RWArenaState {
	MemArena *arena;
	char *buffer;
	uint32_t buffer_size;
	uint32_t io_offset;
} RWArenaState;

SDL_IOStream *SDL_RWArena(MemArena *arena, uint32_t init_buffer_size, RWArenaState *state);
