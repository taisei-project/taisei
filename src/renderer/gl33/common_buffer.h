/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "opengl.h"
#include "../api.h"
#include "../common/cached_buffer.h"

typedef struct CommonBuffer CommonBuffer;

struct CommonBuffer {
	CachedBuffer cachedbuf;

	size_t commited_size;
	GLuint gl_handle;
	GLuint gl_usage_hint;
	uint bindidx;
	char debug_label[R_DEBUG_LABEL_SIZE];

	void (*pre_bind)(CommonBuffer *self);
	void (*post_bind)(CommonBuffer *self);
};

CommonBuffer *gl33_buffer_create(uint bindidx, size_t alloc_size);
void gl33_buffer_init(CommonBuffer *cbuf, size_t capacity, void *data, GLenum usage_hint);
void gl33_buffer_destroy(CommonBuffer *cbuf);
void gl33_buffer_invalidate(CommonBuffer *cbuf);
void gl33_buffer_resize(CommonBuffer *cbuf, size_t new_size);
SDL_IOStream *gl33_buffer_get_stream(CommonBuffer *cbuf);
void gl33_buffer_flush(CommonBuffer *cbuf);

#define GL33_BUFFER_TEMP_BIND(cbuf, code) do { \
	CommonBuffer *_tempbind_cbuf = (cbuf); \
	attr_unused BufferBindingIndex _tempbind_bindidx = 0; \
	attr_unused GLuint _tempbind_buf_saved = 0; \
	if(_tempbind_cbuf->pre_bind != NULL) { \
		assume(_tempbind_cbuf->post_bind != NULL); \
		_tempbind_cbuf->pre_bind(cbuf); \
	} else { \
		_tempbind_bindidx = _tempbind_cbuf->bindidx; \
		_tempbind_buf_saved = gl33_buffer_current(_tempbind_bindidx); \
		gl33_bind_buffer(_tempbind_bindidx, _tempbind_cbuf->gl_handle); \
		gl33_sync_buffer(_tempbind_bindidx); \
	} \
	{ code } \
	if(_tempbind_cbuf->post_bind != NULL) { \
		assume(_tempbind_cbuf->pre_bind != NULL); \
		_tempbind_cbuf->post_bind(cbuf); \
	} else { \
		gl33_bind_buffer(_tempbind_bindidx, _tempbind_buf_saved); \
	} \
} while(0)
