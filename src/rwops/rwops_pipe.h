/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_rwops_rwops_pipe_h
#define IGUARD_rwops_rwops_pipe_h

#include "taisei.h"

#include <SDL.h>

SDL_RWops* SDL_RWpopen(const char *command, const char *mode);
int SDL_RWConvertToPipe(SDL_RWops *stdio_rw);

#endif // IGUARD_rwops_rwops_pipe_h
