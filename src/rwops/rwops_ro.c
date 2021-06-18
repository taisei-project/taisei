/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "rwops_ro.h"
#include "rwops_dummy.h"

static size_t ro_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Read-only stream");
	return 0;
}

SDL_RWops *SDL_RWWrapReadOnly(SDL_RWops *src, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_RWops *rw = SDL_RWWrapDummy(src, autoclose);

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	rw->write = ro_write;

	return rw;
}
