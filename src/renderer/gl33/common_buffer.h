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
#include "../api.h"

typedef struct CommonBuffer {
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
			uint bindidx;
			char debug_label[R_DEBUG_LABEL_SIZE];
		};
	};
} CommonBuffer;

static_assert(
	offsetof(CommonBuffer, stream) == 0,
	"stream should be the first member in CommonBuffer for simplicity"
);

CommonBuffer* gl33_buffer_create(uint bindidx, size_t capacity, GLenum usage_hint, void *data);
void gl33_buffer_destroy(CommonBuffer *cbuf);
void gl33_buffer_invalidate(CommonBuffer *cbuf);
SDL_RWops* gl33_buffer_get_stream(CommonBuffer *cbuf);
void gl33_buffer_flush(CommonBuffer *cbuf);

#define GL33_BUFFER_TEMP_BIND(cbuf, code) do { \
	CommonBuffer *_tempbind_cbuf = (cbuf); \
	BufferBindingIndex _tempbind_bindidx = _tempbind_cbuf->bindidx; \
	GLuint _tempbind_buf_saved = gl33_buffer_current(_tempbind_bindidx); \
	gl33_bind_buffer(_tempbind_bindidx, _tempbind_cbuf->gl_handle); \
	gl33_sync_buffer(_tempbind_bindidx); \
	{ code } \
	gl33_bind_buffer(_tempbind_bindidx, _tempbind_buf_saved); \
} while(0)
