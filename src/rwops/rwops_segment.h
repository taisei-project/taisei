/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef RWSEGMENT_H
#define RWSEGMENT_H

#include <SDL.h>
#include <stdbool.h>

SDL_RWops* SDL_RWWrapSegment(SDL_RWops *src, size_t start, size_t end, bool autoclose);

#endif
