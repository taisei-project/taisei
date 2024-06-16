/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "crap.h"
#include "assert.h"

#include <SDL_thread.h>

static_assert(sizeof(void*) == sizeof(void (*)(void)), "Can't store function pointers in void* :(");

void inherit_missing_pointers(uint num, void *dest[num], void *const base[num]) {
	for(uint i = 0; i < num; ++i) {
		if(dest[i] == NULL) {
			dest[i] = base[i];
		}
	}
}
