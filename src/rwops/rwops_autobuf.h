/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_rwops_rwops_autobuf_h
#define IGUARD_rwops_rwops_autobuf_h

#include "taisei.h"

#include <SDL.h>

SDL_RWops *SDL_RWAutoBuffer(void **ptr, size_t initsize);

#endif // IGUARD_rwops_rwops_autobuf_h
