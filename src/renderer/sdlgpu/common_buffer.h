/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "../common/cached_buffer.h"

typedef struct CommonBuffer CommonBuffer;

struct CommonBuffer {
	CachedBuffer cachedbuf;
	SDL_GPUBuffer *gpubuf;
	SDL_GPUTransferBuffer *transferbuf;
	SDL_GPUBufferUsageFlags usage;
	size_t commited_size;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

CommonBuffer *sdlgpu_buffer_create(SDL_GPUBufferUsageFlags usage, size_t alloc_size);
void sdlgpu_buffer_destroy(CommonBuffer *cbuf);
void sdlgpu_buffer_invalidate(CommonBuffer *cbuf);
void sdlgpu_buffer_resize(CommonBuffer *cbuf, size_t new_size);
SDL_IOStream *sdlgpu_buffer_get_stream(CommonBuffer *cbuf);
void sdlgpu_buffer_flush(CommonBuffer *cbuf);
void sdlgpu_buffer_set_debug_label(CommonBuffer *cbuf, const char *label);
