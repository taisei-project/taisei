/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "rwops_crc32.h"
#include "util.h"

#include <zlib.h>

// #define RWCRC32_DEBUG

#ifdef RWCRC32_DEBUG
	#undef RWCRC32_DEBUG
	#define RWCRC32_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#undef RWCRC32_DEBUG
	#define RWCRC32_DEBUG(...) ((void)0)
#endif

struct crc32_data {
	SDL_RWops *src;
	uint32_t *crc32_ptr;
	bool autoclose;
};

#define DATA(rw) ((struct crc32_data*)((rw)->hidden.unknown.data1))

static int rwcrc32_close(SDL_RWops *rw) {
	if(DATA(rw)->autoclose) {
		SDL_RWclose(DATA(rw)->src);
	}

	free(DATA(rw));
	SDL_FreeRW(rw);
	return 0;
}

static int64_t rwcrc32_seek(SDL_RWops *rw, int64_t offset, int whence) {
	SDL_SetError("Stream is not seekable");
	return -1;
}

static int64_t rwcrc32_size(SDL_RWops *rw) {
	return SDL_RWsize(DATA(rw)->src);
}

static void rwcrc32_update_crc(SDL_RWops *rw, const void *data, size_t size, size_t maxnum, char mode) {
	assert(size <= UINT32_MAX);
	assert(maxnum <= UINT32_MAX);
	assert(size * maxnum <= UINT32_MAX);
	attr_unused uint32_t old = *DATA(rw)->crc32_ptr;
	*DATA(rw)->crc32_ptr = crc32(*DATA(rw)->crc32_ptr, data, size * maxnum);
	RWCRC32_DEBUG("%c %zu bytes: 0x%08x --> 0x%08x", mode, size * maxnum, old, *DATA(rw)->crc32_ptr);
}

static size_t rwcrc32_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	size_t result = SDL_RWread(DATA(rw)->src, ptr, size, maxnum);
	rwcrc32_update_crc(rw, ptr, size, maxnum, 'R');
	return result;
}

static size_t rwcrc32_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	rwcrc32_update_crc(rw, ptr, size, maxnum, 'W');
	return SDL_RWwrite(DATA(rw)->src, ptr, size, maxnum);
}

SDL_RWops *SDL_RWWrapCRC32(SDL_RWops *src, uint32_t *crc32_ptr, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->hidden.unknown.data1 = calloc(1, sizeof(struct crc32_data));
	DATA(rw)->src = src;
	DATA(rw)->crc32_ptr = crc32_ptr;
	DATA(rw)->autoclose = autoclose;

	rw->size = rwcrc32_size;
	rw->seek = rwcrc32_seek;
	rw->close = rwcrc32_close;
	rw->read = rwcrc32_read;
	rw->write = rwcrc32_write;

	return rw;
}
