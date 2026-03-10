/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

typedef struct RWWrapDummyOpts {
	bool autoclose;
	bool readonly;
	bool assert_no_seek;
} RWWrapDummyOpts;

SDL_IOStream *SDL_RWWrapDummy(SDL_IOStream *src, const RWWrapDummyOpts *opts);
