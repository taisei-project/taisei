/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_buffer.h"
#include "gl33.h"

#define STREAM_CBUF(rw) ((CommonBuffer*)rw)

static int64_t gl33_buffer_stream_seek(SDL_RWops *rw, int64_t offset, int whence) {
	CommonBuffer *cbuf = STREAM_CBUF(rw);

	switch(whence) {
		case RW_SEEK_CUR: {
			cbuf->offset += offset;
			break;
		}

		case RW_SEEK_END: {
			cbuf->offset = cbuf->size + offset;
			break;
		}

		case RW_SEEK_SET: {
			cbuf->offset = offset;
			break;
		}
	}

	assert(cbuf->offset <= cbuf->size);
	return cbuf->offset;
}

static int64_t gl33_buffer_stream_size(SDL_RWops *rw) {
	return STREAM_CBUF(rw)->size;
}

static size_t gl33_buffer_stream_write(SDL_RWops *rw, const void *data, size_t size, size_t num) {
	CommonBuffer *cbuf = STREAM_CBUF(rw);
	size_t total_size = size * num;
	size_t offset = cbuf->offset;
	size_t req_bufsize = offset + total_size;

	if(UNLIKELY(req_bufsize > cbuf->size)) {
		gl33_buffer_resize(cbuf, req_bufsize);
		assert(req_bufsize <= cbuf->size);
		assert(offset == cbuf->offset);
	}

	if(LIKELY(total_size > 0)) {
		memcpy(cbuf->cache.buffer + cbuf->offset, data, total_size);
		cbuf->cache.update_begin = umin(cbuf->offset, cbuf->cache.update_begin);
		cbuf->cache.update_end = umax(cbuf->offset + total_size, cbuf->cache.update_end);
		cbuf->offset += total_size;
	}

	return num;
}

static size_t gl33_buffer_stream_read(SDL_RWops *rw, void *data, size_t size, size_t num) {
	SDL_SetError("Can't read from an OpenGL buffer stream");
	return 0;
}

static int gl33_buffer_stream_close(SDL_RWops *rw) {
	SDL_SetError("Can't close an OpenGL buffer stream");
	return -1;
}

SDL_RWops *gl33_buffer_get_stream(CommonBuffer *cbuf) {
	return &cbuf->stream;
}

CommonBuffer *gl33_buffer_create(uint bindidx, size_t alloc_size) {
	CommonBuffer *cbuf = calloc(1, alloc_size);

	cbuf->bindidx = bindidx;
	cbuf->stream.type = SDL_RWOPS_UNKNOWN;
	cbuf->stream.close = gl33_buffer_stream_close;
	cbuf->stream.read = gl33_buffer_stream_read;
	cbuf->stream.write = gl33_buffer_stream_write;
	cbuf->stream.seek = gl33_buffer_stream_seek;
	cbuf->stream.size = gl33_buffer_stream_size;

	glGenBuffers(1, &cbuf->gl_handle);

	return cbuf;
}

void gl33_buffer_init(CommonBuffer *cbuf, size_t capacity, void *data, GLenum usage_hint) {
	assert(cbuf->cache.buffer == NULL);

	size_t data_size = capacity;
	cbuf->size = cbuf->commited_size = capacity = topow2(capacity);
	cbuf->cache.buffer = calloc(1, capacity);
	cbuf->cache.update_begin = capacity;
	cbuf->gl_usage_hint = usage_hint;

	GL33_BUFFER_TEMP_BIND(cbuf, {
		assert(glIsBuffer(cbuf->gl_handle));

		GLenum target = gl33_bindidx_to_glenum(cbuf->bindidx);
		glBufferData(target, cbuf->size, NULL, usage_hint);

		if(data != NULL) {
			glBufferSubData(target, 0, data_size, data);
			memcpy(cbuf->cache.buffer, data, data_size);
		}
	});
}

void gl33_buffer_destroy(CommonBuffer *cbuf) {
	free(cbuf->cache.buffer);
	gl33_buffer_deleted(cbuf);
	glDeleteBuffers(1, &cbuf->gl_handle);
	free(cbuf);
}

void gl33_buffer_invalidate(CommonBuffer *cbuf) {
	// TODO: a better way to set this properly in advance
	cbuf->gl_usage_hint = GL_DYNAMIC_DRAW;
	GL33_BUFFER_TEMP_BIND(cbuf, {
		glBufferData(gl33_bindidx_to_glenum(cbuf->bindidx), cbuf->size, NULL, cbuf->gl_usage_hint);
	});
	cbuf->offset = 0;
}

void gl33_buffer_resize(CommonBuffer *cbuf, size_t new_size) {
	assert(cbuf->cache.buffer != NULL);

	size_t old_size = cbuf->size;
	new_size = topow2(new_size);

	if(UNLIKELY(cbuf->size == new_size)) {
		return;
	}

	log_warn("Resizing buffer %u (%s) from %zu to %zu",
		cbuf->gl_handle, cbuf->debug_label, old_size, new_size
	);

	cbuf->size = new_size;
	cbuf->cache.buffer = realloc(cbuf->cache.buffer, new_size);
	cbuf->cache.update_begin = 0;
	cbuf->cache.update_end = umin(old_size, new_size);

	if(cbuf->offset > new_size) {
		cbuf->offset = new_size;
	}
}

void gl33_buffer_flush(CommonBuffer *cbuf) {
	if(cbuf->cache.update_begin >= cbuf->cache.update_end) {
		return;
	}

	size_t update_size = cbuf->cache.update_end - cbuf->cache.update_begin;
	assert(update_size > 0);

	GL33_BUFFER_TEMP_BIND(cbuf, {
		GLenum target = gl33_bindidx_to_glenum(cbuf->bindidx);

		if(cbuf->size != cbuf->commited_size) {
			log_debug("Resizing buffer %u (%s) from %zu to %zu",
				cbuf->gl_handle, cbuf->debug_label, cbuf->commited_size, cbuf->size
			);
			glBufferData(target, cbuf->size, NULL, cbuf->gl_usage_hint);
			cbuf->commited_size = cbuf->size;
		}

		glBufferSubData(
			target,
			cbuf->cache.update_begin,
			update_size,
			cbuf->cache.buffer + cbuf->cache.update_begin
		);
	});

	cbuf->cache.update_begin = cbuf->size;
	cbuf->cache.update_end = 0;
}
