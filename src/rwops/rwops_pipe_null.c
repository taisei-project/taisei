/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "rwops_pipe.h"

SDL_RWops* SDL_RWpopen(const char *command, const char *mode) {
    SDL_SetError("Not implemented");
    return NULL;
}

int SDL_RWConvertToPipe(SDL_RWops *rw) {
    SDL_SetError("Not implemented");
    return -1;
}
