/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef RWAUTO_H
#define RWAUTO_H

#include <SDL.h>

SDL_RWops* SDL_RWAutoBuffer(void **ptr, size_t initsize);

#endif
