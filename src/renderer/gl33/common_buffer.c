/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
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

	assert(cbuf->offset < cbuf->size);
	return cbuf->offset;
}

static int64_t gl33_buffer_stream_size(SDL_RWops *rw) {
	return STREAM_CBUF(rw)->size;
}

static size_t gl33_buffer_stream_write(SDL_RWops *rw, const void *data, size_t size, size_t num) {
	CommonBuffer *cbuf = STREAM_CBUF(rw);
	size_t total_size = size * num;
	assert(cbuf->offset + total_size <= cbuf->size);

	if(total_size > 0) {
		memcpy(cbuf->cache.buffer + cbuf->offset, data, total_size);
		cbuf->cache.update_begin = min(cbuf->offset, cbuf->cache.update_begin);
		cbuf->cache.update_end = max(cbuf->offset + total_size, cbuf->cache.update_end);
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

SDL_RWops* gl33_buffer_get_stream(CommonBuffer *cbuf) {
	return &cbuf->stream;
}

CommonBuffer* gl33_buffer_create(uint bindidx, size_t capacity, GLenum usage_hint, void *data) {
	CommonBuffer *cbuf = calloc(1, sizeof(CommonBuffer));
	cbuf->size = capacity = topow2(capacity);
	cbuf->cache.buffer = calloc(1, capacity);
	cbuf->cache.update_begin = capacity;
	cbuf->bindidx = bindidx;

	glGenBuffers(1, &cbuf->gl_handle);

	GL33_BUFFER_TEMP_BIND(cbuf, {
		assert(glIsBuffer(cbuf->gl_handle));
		glBufferData(gl33_bindidx_to_glenum(bindidx), capacity, data, usage_hint);
	});

	cbuf->stream.type = SDL_RWOPS_UNKNOWN;
	cbuf->stream.close = gl33_buffer_stream_close;
	cbuf->stream.read = gl33_buffer_stream_read;
	cbuf->stream.write = gl33_buffer_stream_write;
	cbuf->stream.seek = gl33_buffer_stream_seek;
	cbuf->stream.size = gl33_buffer_stream_size;

	return cbuf;
}

void gl33_buffer_destroy(CommonBuffer *cbuf) {
	free(cbuf->cache.buffer);
	gl33_buffer_deleted(cbuf);
	glDeleteBuffers(1, &cbuf->gl_handle);
	free(cbuf);
}

void gl33_buffer_invalidate(CommonBuffer *cbuf) {
	GL33_BUFFER_TEMP_BIND(cbuf, {
		glBufferData(gl33_bindidx_to_glenum(cbuf->bindidx), cbuf->size, NULL, GL_DYNAMIC_DRAW);
	});
	cbuf->offset = 0;
}

void gl33_buffer_flush(CommonBuffer *cbuf) {
	if(cbuf->cache.update_begin >= cbuf->cache.update_end) {
		return;
	}

	size_t update_size = cbuf->cache.update_end - cbuf->cache.update_begin;
	assert(update_size > 0);

	GL33_BUFFER_TEMP_BIND(cbuf, {
		glBufferSubData(
			gl33_bindidx_to_glenum(cbuf->bindidx),
			cbuf->cache.update_begin,
			update_size,
			cbuf->cache.buffer + cbuf->cache.update_begin
		);
	});

	cbuf->cache.update_begin = cbuf->size;
	cbuf->cache.update_end = 0;
}

