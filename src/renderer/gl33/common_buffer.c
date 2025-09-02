/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "common_buffer.h"

#include "gl33.h"

SDL_IOStream *gl33_buffer_get_stream(CommonBuffer *cbuf) {
	return cbuf->cachedbuf.stream;
}

CommonBuffer *gl33_buffer_create(uint bindidx, size_t alloc_size) {
	CommonBuffer *cbuf = mem_alloc(alloc_size);
	cbuf->bindidx = bindidx;
	glGenBuffers(1, &cbuf->gl_handle);
	cachedbuf_init(&cbuf->cachedbuf);
	return cbuf;
}

void gl33_buffer_init(CommonBuffer *cbuf, size_t capacity, void *data, GLenum usage_hint) {
	size_t data_size = capacity;
	cachedbuf_resize(&cbuf->cachedbuf, capacity);
	cbuf->gl_usage_hint = usage_hint;

	GL33_BUFFER_TEMP_BIND(cbuf, {
		assert(glIsBuffer(cbuf->gl_handle));

		GLenum target = gl33_bindidx_to_glenum(cbuf->bindidx);
		glBufferData(target, cbuf->cachedbuf.size, NULL, usage_hint);
		cbuf->commited_size = cbuf->cachedbuf.size;

		if(data != NULL) {
			glBufferSubData(target, 0, data_size, data);
			memcpy(cbuf->cachedbuf.cache, data, data_size);
		}
	});

	log_debug("%s: capacity requested: %zu; actual: %zu", cbuf->debug_label, data_size, cbuf->commited_size);
}

void gl33_buffer_destroy(CommonBuffer *cbuf) {
	gl33_buffer_deleted(cbuf);
	glDeleteBuffers(1, &cbuf->gl_handle);
	cachedbuf_deinit(&cbuf->cachedbuf);
	mem_free(cbuf);
}

void gl33_buffer_invalidate(CommonBuffer *cbuf) {
	// TODO: a better way to set this properly in advance
	cbuf->gl_usage_hint = GL_DYNAMIC_DRAW;
	GL33_BUFFER_TEMP_BIND(cbuf, {
		cbuf->commited_size = cbuf->cachedbuf.size;
		glBufferData(gl33_bindidx_to_glenum(cbuf->bindidx), cbuf->commited_size, NULL, cbuf->gl_usage_hint);
	});
	cbuf->cachedbuf.stream_offset = 0;
}

void gl33_buffer_resize(CommonBuffer *cbuf, size_t new_size) {
	assert(cbuf->cachedbuf.cache != NULL);
	cachedbuf_resize(&cbuf->cachedbuf, new_size);
}

void gl33_buffer_flush(CommonBuffer *cbuf) {
	auto update = cachedbuf_flush(&cbuf->cachedbuf);

	if(!update.size) {
		return;
	}

	GL33_BUFFER_TEMP_BIND(cbuf, {
		GLenum target = gl33_bindidx_to_glenum(cbuf->bindidx);

		if(cbuf->cachedbuf.size != cbuf->commited_size) {
			log_debug("Resizing buffer %u (%s) from %zu to %zu",
				cbuf->gl_handle, cbuf->debug_label, cbuf->commited_size, cbuf->cachedbuf.size
			);
			glBufferData(target, cbuf->cachedbuf.size, cbuf->cachedbuf.cache, cbuf->gl_usage_hint);
			cbuf->commited_size = cbuf->cachedbuf.size;
		} else {
			glBufferSubData(target, update.offset, update.size, update.data);
		}
	});
}
