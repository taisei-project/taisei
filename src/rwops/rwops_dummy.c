/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_dummy.h"

#define DUMMY_SOURCE(rw) ((SDL_RWops*)((rw)->hidden.unknown.data1))
#define DUMMY_AUTOCLOSE(rw) ((bool)((rw)->hidden.unknown.data2))

static int dummy_close(SDL_RWops *rw) {
	if(DUMMY_AUTOCLOSE(rw)) {
		SDL_RWclose(DUMMY_SOURCE(rw));
	}

	SDL_FreeRW(rw);
	return 0;
}

static int64_t dummy_seek(SDL_RWops *rw, int64_t offset, int whence) {
	return SDL_RWseek(DUMMY_SOURCE(rw), offset, whence);
}

static int64_t dummy_size(SDL_RWops *rw) {
	return SDL_RWsize(DUMMY_SOURCE(rw));
}

static size_t dummy_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	return SDL_RWread(DUMMY_SOURCE(rw), ptr, size, maxnum);
}

static size_t dummy_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	return SDL_RWwrite(DUMMY_SOURCE(rw), ptr, size, maxnum);
}

SDL_RWops *SDL_RWWrapDummy(SDL_RWops *src, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->hidden.unknown.data1 = src;
	rw->hidden.unknown.data2 = (void*)(intptr_t)autoclose;
	rw->type = SDL_RWOPS_UNKNOWN;

	rw->size = dummy_size;
	rw->seek = dummy_seek;
	rw->close = dummy_close;
	rw->read = dummy_read;
	rw->write = dummy_write;

	return rw;
}
