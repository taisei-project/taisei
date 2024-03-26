/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_dummy.h"

#define DUMMY_SOURCE(rw) ((SDL_IOStream*)((rw)->hidden.unknown.data1))
#define DUMMY_AUTOCLOSE(rw) ((bool)((rw)->hidden.unknown.data2))

static int dummy_close(SDL_IOStream *rw) {
	if(DUMMY_AUTOCLOSE(rw)) {
		SDL_CloseIO(DUMMY_SOURCE(rw));
	}

	SDL_FreeRW(rw);
	return 0;
}

static int64_t dummy_seek(SDL_IOStream *rw, int64_t offset, int whence) {
	return SDL_SeekIO(DUMMY_SOURCE(rw), offset, whence);
}

static int64_t dummy_size(SDL_IOStream *rw) {
	return SDL_SizeIO(DUMMY_SOURCE(rw));
}

static size_t dummy_read(SDL_IOStream *rw, void *ptr, size_t size,
			 size_t maxnum) {
	return /* FIXME MIGRATION: double-check if you use the returned value of SDL_RWread() */
	SDL_ReadIO(DUMMY_SOURCE(rw), ptr, size * maxnum);
}

static size_t dummy_write(SDL_IOStream *rw, const void *ptr, size_t size,
			  size_t maxnum) {
	return /* FIXME MIGRATION: double-check if you use the returned value of SDL_RWwrite() */
	SDL_WriteIO(DUMMY_SOURCE(rw), ptr, size * maxnum);
}

SDL_IOStream *SDL_RWWrapDummy(SDL_IOStream *src, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStream *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_IOStream));

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
