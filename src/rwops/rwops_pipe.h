/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include <stdbool.h>

SDL_RWops* SDL_RWpopen(const char *command, const char *mode);
int SDL_RWConvertToPipe(SDL_RWops *stdio_rw);
