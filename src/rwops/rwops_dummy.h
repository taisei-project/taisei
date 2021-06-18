/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_rwops_rwops_dummy_h
#define IGUARD_rwops_rwops_dummy_h

#include "taisei.h"

#include <SDL.h>

SDL_RWops *SDL_RWWrapDummy(SDL_RWops *src, bool autoclose);

#endif // IGUARD_rwops_rwops_dummy_h
