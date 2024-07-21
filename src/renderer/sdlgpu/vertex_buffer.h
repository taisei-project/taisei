/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "common_buffer.h"

typedef struct VertexBuffer {
	CommonBuffer cbuf;
} VertexBuffer;

VertexBuffer *sdlgpu_vertex_buffer_create(size_t capacity, void *data);
const char *sdlgpu_vertex_buffer_get_debug_label(VertexBuffer *vbuf);
void sdlgpu_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label);
void sdlgpu_vertex_buffer_destroy(VertexBuffer *vbuf);
void sdlgpu_vertex_buffer_invalidate(VertexBuffer *vbuf);
SDL_IOStream *sdlgpu_vertex_buffer_get_stream(VertexBuffer *vbuf);
void sdlgpu_vertex_buffer_flush(VertexBuffer *vbuf);
