/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "crap.h"
#include "assert.h"

#include <stdlib.h>
#include <string.h>
#include <SDL_thread.h>

void* memdup(const void *src, size_t size) {
	void *data = malloc(size);
	memcpy(data, src, size);
	return data;
}

static_assert(sizeof(void*) == sizeof(void (*)(void)), "Can't store function pointers in void* :(");

void inherit_missing_pointers(uint num, void *dest[num], void *const base[num]) {
	for(uint i = 0; i < num; ++i) {
		if(dest[i] == NULL) {
			dest[i] = base[i];
		}
	}
}

bool is_main_thread(void) {
	static bool initialized = false;
	static SDL_threadID main_thread_id = 0;
	SDL_threadID tid = SDL_ThreadID();

	if(!initialized) {
		main_thread_id = tid;
	}

	return main_thread_id == tid;
}

