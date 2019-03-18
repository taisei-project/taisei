/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_util_io_h
#define IGUARD_util_io_h

#include "taisei.h"

#include <SDL.h>
#include <stdio.h>

char* read_all(const char *filename, int *size);

char* SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize);
char* SDL_RWgets_realloc(SDL_RWops *rwops, char **buf, size_t *bufsize);
size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) attr_printf(2, 3);

// This is for the very few legitimate uses for printf/fprintf that shouldn't be replaced with log_*
void tsfprintf(FILE *out, const char *restrict fmt, ...) attr_printf(2, 3);

char* try_path(const char *prefix, const char *name, const char *ext);

#endif // IGUARD_util_io_h
