/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_zlib.h"

#include "log.h"
#include "rwops_util.h"
#include "util/miscmath.h"

#include <SDL3/SDL.h>
#include <zlib.h>

#define MIN_CHUNK_SIZE 1024

// #define TEST_ZRWOPS
// #define DEBUG_ZRWOPS

#ifdef TEST_ZRWOPS
	#define DEBUG_ZRWOPS
#endif

#ifdef DEBUG_ZRWOPS
	#define PRINT(...) tsfprintf(stdout, __VA_ARGS__)
#else
	#define PRINT(...)
#endif

typedef struct ZData {
	SDL_IOStream *iostream;
	SDL_IOStream *wrapped;

	uint8_t *buffer;
	uint8_t *buffer_ptr;
	size_t buffer_size;

	union {
		// deflate
		struct {
			uint8_t *buffer_aux;
			size_t buffer_aux_size;
		};

		// inflate
		struct {
			size_t buffer_fillsize;
			int64_t uncompressed_size;
		};
	};

	enum {
		TYPE_DEFLATE,
		TYPE_INFLATE,
	} type;

	int window_bits;
	int64_t pos;
	z_stream *stream;

	bool autoclose;
} ZData;

#define TYPENAME(z) ((z)->type == TYPE_DEFLATE ? "a deflate" : "an inflate")

static int64_t common_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZData *z = ctx;

	if(!offset && whence == SDL_IO_SEEK_CUR) {
		return z->pos;
	}

	return SDL_SetError("Can't seek in %s stream", TYPENAME(z));
}

static int64_t common_size(void *ctx) {
	ZData *z = ctx;

	if(z->type == TYPE_INFLATE && z->uncompressed_size >= 0) {
		return z->uncompressed_size;
	}

	return SDL_SetError("Can't get size of %s stream", TYPENAME(z));
}

static void deflate_flush(ZData *z, SDL_IOStatus *status);

static bool common_close(void *ctx) {
	PRINT("common_close\n");
	ZData *z = ctx;

	PRINT("T = %i\n", z->type);

	if(z->type == TYPE_DEFLATE) {
		SDL_IOStatus status;
		deflate_flush(z, &status);
		deflateEnd(z->stream);
		mem_free(z->buffer_aux);
	} else {
		inflateEnd(z->stream);
	}

	mem_free(z->buffer);
	mem_free(z->stream);

	bool result = true;

	if(z->autoclose) {
		result = SDL_CloseIO(z->wrapped);
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

static SDL_IOStream *common_alloc(
	ZData **out_z, SDL_IOStream *wrapped, size_t bufsize, bool autoclose,
	bool writer, bool emulate_seek
) {
	if(bufsize < MIN_CHUNK_SIZE) {
		bufsize = MIN_CHUNK_SIZE;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = common_size,
		.seek = common_seek,
		.close = common_close,
	};

	if(writer) {
		assume(!emulate_seek);
		iface.write = deflate_write;
	} else {
		iface.read = inflate_read;
		if(emulate_seek) {
			iface.seek = inflate_seek_emulated;
		}
	}

	auto z = ALLOC(ZData, {
		.buffer_size = bufsize,
		.wrapped = wrapped,
		.autoclose = autoclose,
		.type = writer ? TYPE_DEFLATE : TYPE_INFLATE,
	});


	SDL_IOStream *io = SDL_OpenIO(&iface, z);

	if(!io) {
		mem_free(z);
		return NULL;
	}

	z->stream = ALLOC(z_stream, {
		.zalloc = zlib_alloc,
		.zfree = zlib_free,
	});

	z->buffer = z->buffer_ptr = mem_alloc(bufsize);
	z->iostream = io;

	*out_z = z;
	return io;
}

#ifdef DEBUG_ZRWOPS
static void printbuf(void *buf, size_t size) {
	tsfprintf(stdout, "[ ");
	for(uint i = 0; i < size; ++i) {
		tsfprintf(stdout, "%02x ", ((uint8_t*)buf)[i]);
	}
	tsfprintf(stdout, "]\n");
}
#else
#define printbuf(buf,size)
#endif

static void deflate_flush(ZData *z, SDL_IOStatus *status) {
	size_t totalsize = z->buffer_ptr - z->buffer;

	PRINT("deflate_flush: %lu\n", totalsize);

	if(!totalsize) {
		return;
	}

	z->stream->next_in = z->buffer;
	z->stream->avail_in = totalsize;

	z->stream->next_out = z->buffer_aux;
	z->stream->avail_out = z->buffer_aux_size;

	int ret = Z_OK;

	while(ret >= 0) {
		PRINT("deflate start: (%p %i %p %i %lu)\n", (void*)z->stream->next_in, z->stream->avail_in, (void*)z->stream->next_out, z->stream->avail_out, totalsize);

		ret = deflate(z->stream, Z_SYNC_FLUSH);

		PRINT("deflate end: %i (%p %i %p %i %lu)\n", ret, (void*)z->stream->next_in, z->stream->avail_in, (void*)z->stream->next_out, z->stream->avail_out, totalsize);

		if(!z->stream->avail_out) {
			SDL_WriteIO(z->wrapped, z->buffer_aux, z->buffer_aux_size);
			*status = SDL_GetIOStatus(z->wrapped);
			z->stream->next_out = z->buffer_aux;
			z->stream->avail_out = z->buffer_aux_size;
		}
	}

	size_t remaining = z->buffer_aux_size - z->stream->avail_out;
	if(remaining) {
		SDL_WriteIO(z->wrapped, z->buffer_aux, remaining);
		*status = SDL_GetIOStatus(z->wrapped);
	}

	z->buffer_ptr = z->buffer;

	PRINT("---\n");
}

static size_t deflate_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	ZData *z = ctx;
	size_t available;
	size_t remaining = size;
	size_t offset = 0;

	while(remaining) {
		available = z->buffer_size - (z->buffer_ptr - z->buffer);
		size_t copysize = remaining > available ? available : remaining;

		PRINT("avail = %lu; copysize = %lu\n", available, copysize);

		if(available) {
			remaining -= copysize;
			memcpy(z->buffer_ptr, (uint8_t*)ptr + offset, copysize);
			printbuf(z->buffer_ptr, copysize);
			offset += copysize;
			z->buffer_ptr += copysize;
			z->pos += copysize;
		} else {
			deflate_flush(z, status);
		}
	}

	return size;
}

static size_t inflate_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	ZData *z = ctx;
	size_t totalsize = size;
	int ret = Z_OK;

	if(!totalsize) {
		return 0;
	}

	z->stream->avail_out = totalsize;
	z->stream->next_out = ptr;

	PRINT("inflate_read()\n");

	while(z->stream->avail_out && ret != Z_STREAM_END) {
		z->stream->avail_in = z->buffer_fillsize - (z->buffer_ptr - z->buffer);

		if(!z->stream->avail_in) {
			z->buffer_fillsize = SDL_ReadIO(z->wrapped, z->buffer, z->buffer_size);
			*status = SDL_GetIOStatus(z->wrapped);  // FIXME not sure if correct
			z->buffer_ptr = z->buffer;
			z->stream->avail_in = z->buffer_fillsize - (z->buffer_ptr - z->buffer);
		}

		z->stream->next_in = z->buffer_ptr;

		PRINT(" -- begin read %i --- \n", z->stream->avail_in);
		printbuf(z->stream->next_in, z->stream->avail_in);
		PRINT(" -- end read --- \n");

		switch(ret = inflate(z->stream, Z_NO_FLUSH)) {
			case Z_OK:
			case Z_STREAM_END:
				PRINT("read ok\n");
				break;

			default:
				PRINT("inflate error: %i\n", ret);
				SDL_SetError("inflate error: %i", ret);
				log_debug("%s", SDL_GetError());
				ret = Z_STREAM_END;
				*status = SDL_IO_STATUS_ERROR;
				break;
		}

		z->buffer_ptr = z->stream->next_in;
	}

	z->pos += (totalsize - z->stream->avail_out);
	return totalsize - z->stream->avail_out;
}

static SDL_IOStream *wrap_writer(
	SDL_IOStream *src, size_t bufsize,
	bool autoclose, int clevel, int window_bits
) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZData *z;
	SDL_IOStream *io = common_alloc(&z, src, bufsize, autoclose, true, false);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	z->buffer_aux_size = z->buffer_size;
	z->buffer_aux = mem_alloc(z->buffer_aux_size);
	z->window_bits = window_bits;

	if(clevel >= 0) {
		clevel = clamp(clevel, Z_BEST_SPEED, Z_BEST_COMPRESSION);
	}

	int status = deflateInit2(
		z->stream,
		clevel,
		Z_DEFLATED,
		window_bits,
		8,
		Z_DEFAULT_STRATEGY
	);

	if(status != Z_OK) {
		SDL_SetError("deflateInit2() failed: %i", status);
		SDL_CloseIO(io);
		return NULL;
	}

	return io;
}

static int inflate_reopen(void *ctx) {
	ZData *z = ctx;

	int64_t srcpos = SDL_SeekIO(z->wrapped, 0, SDL_IO_SEEK_SET);

	if(srcpos < 0) {
		return srcpos;
	}

	assert(srcpos == 0);

	z->pos = 0;
	z->buffer_ptr = z->buffer;
	z->buffer_fillsize = 0;

	int status = inflateReset2(z->stream, z->window_bits);

	if(status != Z_OK) {
		return SDL_SetError("inflateReset2() failed: %i", status);
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

static SDL_IOStream *wrap_reader(
	SDL_IOStream *src, size_t bufsize,
	bool autoclose,
	int64_t uncompressed_size, int window_bits,
	bool emulate_seek
) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZData *z;
	SDL_IOStream *rw = common_alloc(&z, src, bufsize, autoclose, false, emulate_seek);

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	z->window_bits = window_bits;
	z->uncompressed_size = uncompressed_size;

	int status = inflateInit2(z->stream, window_bits);

	if(UNLIKELY(status != Z_OK)) {
		SDL_SetError("inflateInit2() failed: %i", status);
		SDL_CloseIO(rw);
		return NULL;
	}

	return rw;
}

SDL_IOStream *SDL_RWWrapZlibReader(SDL_IOStream *src, size_t bufsize,
				   bool autoclose) {
	return wrap_reader(src, bufsize, autoclose, -1, 15, false);
}

SDL_IOStream *SDL_RWWrapZlibReaderSeekable(SDL_IOStream *src,
					   int64_t uncompressed_size,
					   size_t bufsize, bool autoclose) {
	return wrap_reader(src, bufsize, autoclose, uncompressed_size, 15, true);
}

SDL_IOStream *SDL_RWWrapInflateReader(SDL_IOStream *src, size_t bufsize,
				      bool autoclose) {
	return wrap_reader(src, bufsize, autoclose, -1, 15, false);
}

SDL_IOStream *SDL_RWWrapInflateReaderSeekable(SDL_IOStream *src,
					      int64_t uncompressed_size,
					      size_t bufsize, bool autoclose) {
	return wrap_reader(src, bufsize, autoclose, uncompressed_size, -15, true);
}

SDL_IOStream *SDL_RWWrapZlibWriter(SDL_IOStream *src, int clevel,
				   size_t bufsize, bool autoclose) {
	return wrap_writer(src, bufsize, autoclose, clevel, 15);
}

SDL_IOStream *SDL_RWWrapDeflateWriter(SDL_IOStream *src, int clevel,
				      size_t bufsize, bool autoclose) {
	return wrap_writer(src, bufsize, autoclose, clevel, -15);
}
