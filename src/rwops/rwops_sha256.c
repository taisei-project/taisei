/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_sha256.h"

struct sha256_data {
	SDL_IOStream *src;
	SHA256State *sha256;
	bool autoclose;
};

#define DATA(rw) ((struct sha256_data*)((rw)->hidden.unknown.data1))

static int rwsha256_close(SDL_IOStream *rw) {
	if(DATA(rw)->autoclose) {
		SDL_CloseIO(DATA(rw)->src);
	}

	mem_free(DATA(rw));
	SDL_FreeRW(rw);
	return 0;
}

static int64_t rwsha256_seek(SDL_IOStream *rw, int64_t offset, int whence) {
	if(!offset && whence == SDL_IO_SEEK_CUR) {
		return SDL_SeekIO(DATA(rw)->src, offset, whence);
	}

	SDL_SetError("Stream is not seekable");
	return -1;
}

static int64_t rwsha256_size(SDL_IOStream *rw) {
	return SDL_SizeIO(DATA(rw)->src);
}

static void rwsha256_update_sha256(SDL_IOStream *rw, const void *data,
				   size_t size, size_t maxnum) {
	assert(size <= UINT32_MAX);
	assert(maxnum <= UINT32_MAX);
	assert(size * maxnum <= UINT32_MAX);
	sha256_update(DATA(rw)->sha256, data, size * maxnum);
}

static size_t rwsha256_read(SDL_IOStream *rw, void *ptr, size_t size,
			    size_t maxnum) {
	size_t result = /* FIXME MIGRATION: double-check if you use the returned value of SDL_ReadIO() */
		SDL_ReadIO(DATA(rw)->src, ptr, size * maxnum);
	rwsha256_update_sha256(rw, ptr, size, result);
	return result;
}

static size_t rwsha256_write(SDL_IOStream *rw, const void *ptr, size_t size,
			     size_t maxnum) {
	rwsha256_update_sha256(rw, ptr, size, maxnum);
	return /* FIXME MIGRATION: double-check if you use the returned value of SDL_WriteIO() */
	SDL_WriteIO(DATA(rw)->src, ptr, size * maxnum);
}

SDL_IOStream *SDL_RWWrapSHA256(SDL_IOStream *src, SHA256State *sha256,
			       bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStream *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_IOStream));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->hidden.unknown.data1 = ALLOC(struct sha256_data);
	DATA(rw)->src = src;
	DATA(rw)->sha256 = sha256;
	DATA(rw)->autoclose = autoclose;

	rw->size = rwsha256_size;
	rw->seek = rwsha256_seek;
	rw->close = rwsha256_close;
	rw->read = rwsha256_read;
	rw->write = rwsha256_write;

	return rw;
}
