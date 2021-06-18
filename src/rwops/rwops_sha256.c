/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "rwops_sha256.h"
#include "util.h"

struct sha256_data {
	SDL_RWops *src;
	SHA256State *sha256;
	bool autoclose;
};

#define DATA(rw) ((struct sha256_data*)((rw)->hidden.unknown.data1))

static int rwsha256_close(SDL_RWops *rw) {
	if(DATA(rw)->autoclose) {
		SDL_RWclose(DATA(rw)->src);
	}

	free(DATA(rw));
	SDL_FreeRW(rw);
	return 0;
}

static int64_t rwsha256_seek(SDL_RWops *rw, int64_t offset, int whence) {
	if(!offset && whence == RW_SEEK_CUR) {
		return SDL_RWseek(DATA(rw)->src, offset, whence);
	}

	SDL_SetError("Stream is not seekable");
	return -1;
}

static int64_t rwsha256_size(SDL_RWops *rw) {
	return SDL_RWsize(DATA(rw)->src);
}

static void rwsha256_update_sha256(SDL_RWops *rw, const void *data, size_t size, size_t maxnum) {
	assert(size <= UINT32_MAX);
	assert(maxnum <= UINT32_MAX);
	assert(size * maxnum <= UINT32_MAX);
	sha256_update(DATA(rw)->sha256, data, size * maxnum);
}

static size_t rwsha256_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	size_t result = SDL_RWread(DATA(rw)->src, ptr, size, maxnum);
	rwsha256_update_sha256(rw, ptr, size, result);
	return result;
}

static size_t rwsha256_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	rwsha256_update_sha256(rw, ptr, size, maxnum);
	return SDL_RWwrite(DATA(rw)->src, ptr, size, maxnum);
}

SDL_RWops *SDL_RWWrapSHA256(SDL_RWops *src, SHA256State *sha256, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->hidden.unknown.data1 = calloc(1, sizeof(struct sha256_data));
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
