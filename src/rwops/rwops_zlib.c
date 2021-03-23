/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <SDL.h>
#include <zlib.h>

#include "rwops_zlib.h"
#include "util.h"

#define MIN_CHUNK_SIZE 1024

#define ZDATA(rw) ((ZData*)((rw)->hidden.unknown.data1))
#define TYPENAME(rw) (ZDATA(rw)->type == TYPE_DEFLATE ? "a deflate" : "an inflate")

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
		};
	};

	enum {
		TYPE_DEFLATE,
		TYPE_INFLATE,
	} type;

	size_t pos;
	z_stream *stream;

	SDL_RWops *wrapped;
	bool autoclose;
} ZData;

static int64_t common_seek(SDL_RWops *rw, int64_t offset, int whence) {
	if(!offset && whence == RW_SEEK_CUR) {
		return ZDATA(rw)->pos;
	}

	return SDL_SetError("Can't seek in %s stream", TYPENAME(rw));
}

static int64_t common_size(SDL_RWops *rw) {
	return SDL_SetError("Can't get size of %s stream", TYPENAME(rw));
}

static void deflate_flush(ZData *z);

static int common_close(SDL_RWops *rw) {
	PRINT("common_close\n");

	if(rw) {
		ZData *z = ZDATA(rw);

		PRINT("T = %i\n", z->type);

		if(z->type == TYPE_DEFLATE) {
			deflate_flush(z);
			deflateEnd(z->stream);
			free(z->buffer_aux);
		} else {
			inflateEnd(z->stream);
		}

		free(z->buffer);
		free(z->stream);

		if(z->autoclose) {
			SDL_RWclose(z->wrapped);
		}

		free(z);
		SDL_FreeRW(rw);
	}

	return 0;
}

static SDL_RWops* common_alloc(SDL_RWops *wrapped, size_t bufsize, bool autoclose) {
	SDL_RWops *rw = SDL_AllocRW();

	if(!rw) {
		return NULL;
	}

	if(bufsize < MIN_CHUNK_SIZE) {
		bufsize = MIN_CHUNK_SIZE;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->size = common_size;
	rw->seek = common_seek;
	rw->close = common_close;

	ZData *z = calloc(1, sizeof(ZData));
	z->stream = calloc(1, sizeof(z_stream));
	z->buffer_size = bufsize;
	z->buffer = z->buffer_ptr = calloc(1, bufsize);

	z->wrapped = wrapped;
	z->autoclose = autoclose;

	rw->hidden.unknown.data1 = z;

	return rw;
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

static size_t deflate_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Can't read from a deflate stream");
	return 0;
}

static void deflate_flush(ZData *z) {
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
			SDL_RWwrite(z->wrapped, z->buffer_aux, 1, z->buffer_aux_size);
			z->stream->next_out = z->buffer_aux;
			z->stream->avail_out = z->buffer_aux_size;
		}
	}

	SDL_RWwrite(z->wrapped, z->buffer_aux, 1, z->buffer_aux_size - z->stream->avail_out);
	z->buffer_ptr = z->buffer;

	PRINT("---\n");
}

static size_t deflate_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	ZData *z = ZDATA(rw);
	size_t totalsize = size * maxnum;
	size_t available;
	size_t remaining = totalsize;
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
			deflate_flush(z);
		}
	}

	return maxnum;
}

static size_t inflate_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	ZData *z = ZDATA(rw);
	size_t totalsize = size * maxnum;
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
			z->buffer_fillsize = SDL_RWread(z->wrapped, z->buffer, 1, z->buffer_size);
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
				ret = Z_STREAM_END;
				break;
		}

		z->buffer_ptr = z->stream->next_in;
	}

	z->pos += (totalsize - z->stream->avail_out);
	return (totalsize - z->stream->avail_out) / size;
}

static size_t inflate_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Can't write to an inflate stream");
	return 0;
}

SDL_RWops* SDL_RWWrapZReader(SDL_RWops *src, size_t bufsize, bool autoclose) {
	if(!src) {
		return NULL;
	}

	SDL_RWops *rw = common_alloc(src, bufsize, autoclose);

	if(!rw) {
		return NULL;
	}

	rw->read = inflate_read;
	rw->write = inflate_write;

	ZData *z = ZDATA(rw);
	z->type = TYPE_INFLATE;

	inflateInit(ZDATA(rw)->stream);

	return rw;
}

SDL_RWops* SDL_RWWrapZWriter(SDL_RWops *src, size_t bufsize, bool autoclose) {
	if(!src) {
		return NULL;
	}

	SDL_RWops *rw = common_alloc(src, bufsize, autoclose);

	if(!rw) {
		return NULL;
	}

	rw->read = deflate_read;
	rw->write = deflate_write;

	ZData *z = ZDATA(rw);
	z->type = TYPE_DEFLATE;
	z->buffer_aux_size = z->buffer_size;
	z->buffer_aux = calloc(1, z->buffer_aux_size);

	deflateInit(z->stream, Z_DEFAULT_COMPRESSION);

	return rw;
}
