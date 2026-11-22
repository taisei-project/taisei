/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/arena.h"

#include <SDL3/SDL.h>

typedef struct IOStreamArenaState {
	MemArena *arena;
	char *buffer;
	uint32_t buffer_size;
	uint32_t io_offset;
} IOStreamArenaState;

SDL_IOStream *iostream_arena(MemArena *arena, uint32_t init_buffer_size, IOStreamArenaState *state);
