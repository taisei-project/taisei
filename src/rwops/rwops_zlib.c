/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_zlib.h"

#include "log.h"
#include "rwops_util.h"
#include "util/miscmath.h"

#include <SDL3/SDL.h>
#include <zlib.h>

#define ZLIB_BUFFER_SIZE 32768

typedef struct ZData {
	SDL_IOStream *iostream;
	SDL_IOStream *wrapped;

	struct {
		size_t buffer_fillsize;
		size_t buffer_pos;
		int64_t uncompressed_size;
	} inflate;

	z_stream stream;
	int64_t pos;
	int window_bits;
	bool is_deflate;
	bool autoclose;

	uint8_t buffer[ZLIB_BUFFER_SIZE];
} ZData;

#define TYPENAME(z) ((z)->is_deflate ? "a deflate" : "an inflate")

static int64_t common_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZData *z = ctx;

	if(!offset && whence == SDL_IO_SEEK_CUR) {
		return z->pos;
	}

	return SDL_SetError("Can't seek in %s stream", TYPENAME(z));
}

static int64_t common_size(void *ctx) {
	ZData *z = ctx;

	if(!z->is_deflate && z->inflate.uncompressed_size >= 0) {
		return z->inflate.uncompressed_size;
	}

	SDL_SetError("Can't get size of %s stream", TYPENAME(z));
	return -1;
}

static bool deflate_do_flush(ZData *z, int flush, SDL_IOStatus *status);

static bool common_close(void *ctx) {
	ZData *z = ctx;
	bool result = true;

	if(z->is_deflate) {
		SDL_IOStatus status = SDL_IO_STATUS_READY;
		result = deflate_do_flush(z, Z_FINISH, &status);
		deflateEnd(&z->stream);
	} else {
		inflateEnd(&z->stream);
	}

	if(z->autoclose) {
		result = SDL_CloseIO(z->wrapped) && result;
	}

	mem_free(z);
	return result;
}

static void *zlib_alloc(void *opaque, unsigned int items, unsigned int size) {
	return mem_alloc_array(items, size);
}

static void zlib_free(void *opaque, void *address) {
	return mem_free(address);
}

static int64_t inflate_seek_emulated(void *ctx, int64_t offset, SDL_IOWhence whence);
static size_t inflate_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status);
static size_t deflate_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status);
static bool deflate_flush(void *ctx, SDL_IOStatus *status);

static SDL_IOStream *common_alloc(
	ZData **out_z, SDL_IOStream *wrapped, bool autoclose, bool writer, int window_bits
) {
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = common_size,
		.seek = common_seek,
		.close = common_close,
	};

	if(writer) {
		iface.write = deflate_write;
		iface.flush = deflate_flush;
	} else {
		iface.read = inflate_read;
		iface.seek = inflate_seek_emulated;
	}

	auto z = ALLOC(ZData, {
		.wrapped = wrapped,
		.autoclose = autoclose,
		.is_deflate = writer,
		.stream = {
			.zalloc = zlib_alloc,
			.zfree = zlib_free,
		},
		.window_bits = window_bits,
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, z);

	if(!io) {
		mem_free(z);
		return NULL;
	}

	z->iostream = io;
	*out_z = z;

	auto props = SDL_GetIOProperties(io);
	SDL_SetPointerProperty(props, PROP_IOSTREAM_WRAPPED_STREAM, wrapped);

	return io;
}

static bool deflate_do_flush(ZData *z, int flush, SDL_IOStatus *status) {
	z->stream.next_in = NULL;
	z->stream.avail_in = 0;
	z->stream.next_out = z->buffer;
	z->stream.avail_out = ZLIB_BUFFER_SIZE;

	int ret;

	do {
		ret = deflate(&z->stream, flush);

		if(UNLIKELY(ret < 0 && ret != Z_BUF_ERROR)) {
			SDL_SetError("deflate() failed: %i", ret);
			log_debug("%s", SDL_GetError());
			*status = SDL_IO_STATUS_ERROR;
			return false;
		}

		size_t produced = ZLIB_BUFFER_SIZE - z->stream.avail_out;

		if(produced > 0) {
			size_t written = SDL_WriteIO(z->wrapped, z->buffer, produced);

			if(UNLIKELY(written != produced)) {
				*status = SDL_GetIOStatus(z->wrapped);
				return false;
			}

			z->stream.next_out = z->buffer;
			z->stream.avail_out = ZLIB_BUFFER_SIZE;
		}
	} while(flush == Z_FINISH ? ret != Z_STREAM_END : z->stream.avail_out == 0);

	return true;
}

static bool deflate_flush(void *ctx, SDL_IOStatus *status) {
	return deflate_do_flush(ctx, Z_SYNC_FLUSH, status);
}

static size_t deflate_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	ZData *z = ctx;

	z->stream.next_in = (void*)ptr;
	z->stream.avail_in = size;
	z->stream.next_out = z->buffer;
	z->stream.avail_out = ZLIB_BUFFER_SIZE;

	while(z->stream.avail_in > 0) {
		int ret = deflate(&z->stream, Z_NO_FLUSH);

		if(UNLIKELY(ret < 0 && ret != Z_BUF_ERROR)) {
			SDL_SetError("deflate() failed: %i", ret);
			log_debug("%s", SDL_GetError());
			*status = SDL_IO_STATUS_ERROR;
			return 0;
		}

		size_t produced = ZLIB_BUFFER_SIZE - z->stream.avail_out;

		if(produced > 0) {
			size_t written = SDL_WriteIO(z->wrapped, z->buffer, produced);

			if(UNLIKELY(written != produced)) {
				*status = SDL_GetIOStatus(z->wrapped);
				return 0;
			}

			z->stream.next_out = z->buffer;
			z->stream.avail_out = ZLIB_BUFFER_SIZE;
		}
	}

	z->pos += size;
	return size;
}

static size_t inflate_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	ZData *z = ctx;

	if(!size) {
		return 0;
	}

	z->stream.avail_out = size;
	z->stream.next_out = ptr;

	int ret = Z_OK;

	while(z->stream.avail_out > 0 && ret != Z_STREAM_END) {
		size_t avail_in = z->inflate.buffer_fillsize - z->inflate.buffer_pos;

		if(!avail_in) {
			z->inflate.buffer_fillsize = SDL_ReadIO(z->wrapped, z->buffer, ZLIB_BUFFER_SIZE);
			z->inflate.buffer_pos = 0;
			avail_in = z->inflate.buffer_fillsize;

			if(avail_in == 0) {
				*status = SDL_GetIOStatus(z->wrapped);
				break;
			}
		}

		z->stream.next_in = z->buffer + z->inflate.buffer_pos;
		z->stream.avail_in = avail_in;

		ret = inflate(&z->stream, Z_NO_FLUSH);
		z->inflate.buffer_pos = z->inflate.buffer_fillsize - z->stream.avail_in;

		if(UNLIKELY(ret != Z_OK && ret != Z_STREAM_END)) {
			SDL_SetError("inflate() failed: %i", ret);
			log_debug("%s", SDL_GetError());
			*status = SDL_IO_STATUS_ERROR;
			break;
		}
	}

	size_t bytes_read = size - z->stream.avail_out;
	z->pos += bytes_read;
	return bytes_read;
}

static int inflate_reopen(void *ctx) {
	ZData *z = ctx;

	int64_t srcpos = SDL_SeekIO(z->wrapped, 0, SDL_IO_SEEK_SET);

	if(srcpos < 0) {
		return srcpos;
	}

	assert(srcpos == 0);

	z->pos = 0;
	z->inflate.buffer_pos = 0;
	z->inflate.buffer_fillsize = 0;

	int ret = inflateReset2(&z->stream, z->window_bits);

	if(ret != Z_OK) {
		return SDL_SetError("inflateReset2() failed: %i", ret);
	}

	return 0;
}

static int64_t inflate_seek_emulated(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZData *z = ctx;
	char buf[1024];

	return rwutil_seek_emulated(
		z->iostream, offset, whence,
		&z->pos, inflate_reopen, z, sizeof(buf), buf
	);
}

static SDL_IOStream *wrap_writer(
	SDL_IOStream *src, int clevel, bool autoclose, int window_bits
) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZData *z;
	SDL_IOStream *io = common_alloc(&z, src, autoclose, true, window_bits);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	if(clevel >= 0) {
		clevel = clamp(clevel, Z_BEST_SPEED, Z_BEST_COMPRESSION);
	}

	int ret = deflateInit2(&z->stream, clevel, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);

	if(UNLIKELY(ret != Z_OK)) {
		SDL_SetError("deflateInit2() failed: %i", ret);
		SDL_CloseIO(io);
		return NULL;
	}

	return io;
}

static SDL_IOStream *wrap_reader(
	SDL_IOStream *src, int64_t uncompressed_size, bool autoclose, int window_bits
) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZData *z;
	SDL_IOStream *rw = common_alloc(&z, src, autoclose, false, window_bits);

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	z->inflate.uncompressed_size = uncompressed_size;

	int ret = inflateInit2(&z->stream, window_bits);

	if(UNLIKELY(ret != Z_OK)) {
		SDL_SetError("inflateInit2() failed: %i", ret);
		SDL_CloseIO(rw);
		return NULL;
	}

	return rw;
}

SDL_IOStream *SDL_RWWrapZlibReader(SDL_IOStream *src, int64_t uncompressed_size, bool autoclose) {
	return wrap_reader(src, uncompressed_size, autoclose, 15);
}

SDL_IOStream *SDL_RWWrapInflateReader(SDL_IOStream *src, int64_t uncompressed_size, bool autoclose) {
	return wrap_reader(src, uncompressed_size, autoclose, -15);
}

SDL_IOStream *SDL_RWWrapZlibWriter(SDL_IOStream *src, int clevel, bool autoclose) {
	return wrap_writer(src, clevel, autoclose, 15);
}

SDL_IOStream *SDL_RWWrapDeflateWriter(SDL_IOStream *src, int clevel, bool autoclose) {
	return wrap_writer(src, clevel, autoclose, -15);
}
