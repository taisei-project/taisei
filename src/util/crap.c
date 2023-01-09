/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

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

SDL_threadID main_thread_id = 0;

bool is_main_thread(void) {
	if(main_thread_id == 0) {
		return true;
	}

	SDL_threadID tid = SDL_ThreadID();
	return main_thread_id == tid;
}
