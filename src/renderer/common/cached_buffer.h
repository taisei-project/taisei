/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

typedef struct CachedBuffer CachedBuffer;

struct CachedBuffer {
	SDL_IOStream *stream;
	char *cache;
	size_t size;
	size_t update_begin;
	size_t update_end;
	size_t stream_offset;
};

typedef struct CachedBufferUpdate {
	const char *data;
	size_t size;
	size_t offset;
} CachedBufferUpdate;

void cachedbuf_init(CachedBuffer *cbuf);
void cachedbuf_deinit(CachedBuffer *cbuf);
void cachedbuf_resize(CachedBuffer *cbuf, size_t newsize);
CachedBufferUpdate cachedbuf_flush(CachedBuffer *cbuf);
