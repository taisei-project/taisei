/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "cached_buffer.h"

#include "util/miscmath.h"

static int64_t cachedbuf_stream_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	CachedBuffer *cbuf = ctx;

	switch(whence) {
		case SDL_IO_SEEK_CUR : {
			cbuf->stream_offset += offset;
			break;
		}

		case SDL_IO_SEEK_END : {
			cbuf->stream_offset = cbuf->size + offset;
			break;
		}

		case SDL_IO_SEEK_SET : {
			cbuf->stream_offset = offset;
			break;
		}
	}

	assert(cbuf->stream_offset <= cbuf->size);
	return cbuf->stream_offset;
}

static int64_t cachedbuf_stream_size(void *ctx) {
	CachedBuffer *cbuf = ctx;
	return cbuf->size;
}

static size_t cachedbuf_stream_write(
	void *ctx, const void *data, size_t size, SDL_IOStatus *status
) {
	CachedBuffer *cbuf = ctx;
	size_t offset = cbuf->stream_offset;
	size_t req_bufsize = offset + size;

	if(UNLIKELY(req_bufsize > cbuf->size)) {
		cachedbuf_resize(cbuf, req_bufsize);
		assert(req_bufsize <= cbuf->size);
		assert(offset == cbuf->stream_offset);
	}

	if(LIKELY(size > 0)) {
		memcpy(cbuf->cache + cbuf->stream_offset, data, size);
		cbuf->update_begin = min(cbuf->stream_offset, cbuf->update_begin);
		cbuf->update_end = max(cbuf->stream_offset + size, cbuf->update_end);
		cbuf->stream_offset += size;
	}

	return size;
}

void cachedbuf_init(CachedBuffer *cbuf) {
	*cbuf = (CachedBuffer) {
		.stream = NOT_NULL(SDL_OpenIO(&(SDL_IOStreamInterface) {
			.version = sizeof(SDL_IOStreamInterface),
			.write = cachedbuf_stream_write,
			.seek = cachedbuf_stream_seek,
			.size = cachedbuf_stream_size,
		}, cbuf))
	};
}

void cachedbuf_deinit(CachedBuffer *cbuf) {
	SDL_CloseIO(cbuf->stream);
	mem_free(cbuf->cache);
}

void cachedbuf_resize(CachedBuffer *cbuf, size_t new_size) {
	new_size = topow2(new_size);

	if(new_size == cbuf->size) {
		return;
	}

	cbuf->size = new_size;
	cbuf->cache = mem_realloc(cbuf->cache, new_size);
	cbuf->update_begin = min(cbuf->update_begin, new_size);
	cbuf->update_end = min(cbuf->update_end, new_size);
	cbuf->stream_offset = min(cbuf->stream_offset, new_size);
}

CachedBufferUpdate cachedbuf_flush(CachedBuffer *cbuf) {
	CachedBufferUpdate u = {
		.offset = cbuf->update_begin,
		.size = cbuf->update_end - min(cbuf->update_begin, cbuf->update_end),
		.data = cbuf->cache + cbuf->update_begin,
	};

	cbuf->update_begin = 0;
	cbuf->update_end = 0;

	return u;
}
