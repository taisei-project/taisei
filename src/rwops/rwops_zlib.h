/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_rwops_rwops_zlib_h
#define IGUARD_rwops_rwops_zlib_h

#include "taisei.h"

#include <SDL.h>

SDL_RWops *SDL_RWWrapZReader(SDL_RWops *src, size_t bufsize, bool autoclose);
SDL_RWops *SDL_RWWrapZWriter(SDL_RWops *src, size_t bufsize, bool autoclose);

#endif // IGUARD_rwops_rwops_zlib_h
