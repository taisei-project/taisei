/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "opengl.h"
#include <SDL.h>

typedef struct VertexBuffer {
	union {
		SDL_RWops stream;
		struct {
			char padding[offsetof(SDL_RWops, hidden)];

			struct {
				char *buffer;
				size_t update_begin;
				size_t update_end;
			} cache;

			size_t offset;
			size_t size;
			GLuint gl_handle;
			char debug_label[R_DEBUG_LABEL_SIZE];
		};
	};
} VertexBuffer;

static_assert(
	offsetof(VertexBuffer, stream) == 0,
	"stream should be the first member in VertexBuffer for simplicity"
);

VertexBuffer* gl33_vertex_buffer_create(size_t capacity, void *data);
const char* gl33_vertex_buffer_get_debug_label(VertexBuffer *vbuf);
void gl33_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label);
void gl33_vertex_buffer_destroy(VertexBuffer *vbuf);
void gl33_vertex_buffer_invalidate(VertexBuffer *vbuf);
SDL_RWops* gl33_vertex_buffer_get_stream(VertexBuffer *vbuf);
void gl33_vertex_buffer_flush(VertexBuffer *vbuf);
