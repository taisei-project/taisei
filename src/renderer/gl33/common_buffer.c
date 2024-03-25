/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "common_buffer.h"

#include "gl33.h"

static int64_t gl33_buffer_stream_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	CommonBuffer *cbuf = ctx;

	switch(whence) {
		case SDL_IO_SEEK_CUR : {
			cbuf->offset += offset;
			break;
		}

		case SDL_IO_SEEK_END : {
			cbuf->offset = cbuf->size + offset;
			break;
		}

		case SDL_IO_SEEK_SET : {
			cbuf->offset = offset;
			break;
		}
	}

	assert(cbuf->offset <= cbuf->size);
	return cbuf->offset;
}

static int64_t gl33_buffer_stream_size(void *ctx) {
	CommonBuffer *cbuf = ctx;
	return cbuf->size;
}

static size_t gl33_buffer_stream_write(
	void *ctx, const void *data, size_t size, SDL_IOStatus *status
) {
	CommonBuffer *cbuf = ctx;
	size_t offset = cbuf->offset;
	size_t req_bufsize = offset + size;

	if(UNLIKELY(req_bufsize > cbuf->size)) {
		gl33_buffer_resize(cbuf, req_bufsize);
		assert(req_bufsize <= cbuf->size);
		assert(offset == cbuf->offset);
	}

	if(LIKELY(size > 0)) {
		memcpy(cbuf->cache.buffer + cbuf->offset, data, size);
		cbuf->cache.update_begin = min(cbuf->offset, cbuf->cache.update_begin);
		cbuf->cache.update_end = max(cbuf->offset + size, cbuf->cache.update_end);
		cbuf->offset += size;
	}

	return size;
}

SDL_IOStream *gl33_buffer_get_stream(CommonBuffer *cbuf) {
	return cbuf->stream;
}

CommonBuffer *gl33_buffer_create(uint bindidx, size_t alloc_size) {
	CommonBuffer *cbuf = mem_alloc(alloc_size);
	cbuf->bindidx = bindidx;
	glGenBuffers(1, &cbuf->gl_handle);
	cbuf->stream = NOT_NULL(SDL_OpenIO(&(SDL_IOStreamInterface) {
		.version = sizeof(SDL_IOStreamInterface),
		.write = gl33_buffer_stream_write,
		.seek = gl33_buffer_stream_seek,
		.size = gl33_buffer_stream_size,
	}, cbuf));
	return cbuf;
}

void gl33_buffer_init_cache(CommonBuffer *cbuf, size_t capacity) {
	capacity = topow2(capacity);

	if(cbuf->size != capacity || !cbuf->cache.buffer) {
		cbuf->cache.buffer = mem_realloc(cbuf->cache.buffer, capacity);
		cbuf->size = cbuf->commited_size = capacity;
		cbuf->cache.update_begin = capacity;
	}
}

void gl33_buffer_init(CommonBuffer *cbuf, size_t capacity, void *data, GLenum usage_hint) {
	size_t data_size = capacity;
	gl33_buffer_init_cache(cbuf, capacity);
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
	mem_free(cbuf->cache.buffer);
	gl33_buffer_deleted(cbuf);
	glDeleteBuffers(1, &cbuf->gl_handle);
	SDL_CloseIO(cbuf->stream);
	mem_free(cbuf);
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
	cbuf->cache.buffer = mem_realloc(cbuf->cache.buffer, new_size);
	cbuf->cache.update_begin = 0;
	cbuf->cache.update_end = min(old_size, new_size);

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
