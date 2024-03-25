/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include <stdio.h>

char *SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize) attr_nonnull_all;
char *SDL_RWgets_realloc(SDL_RWops *rwops, char **buf, size_t *bufsize) attr_nonnull_all;
size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) attr_printf(2, 3) attr_nonnull_all;
void *SDL_RWreadAll(SDL_RWops *rwops, size_t *out_size, size_t max_size) attr_nonnull_all;
void SDL_RWsync(SDL_RWops *rwops);

// This is for the very few legitimate uses for printf/fprintf that shouldn't be replaced with log_*
void tsfprintf(FILE *out, const char *restrict fmt, ...) attr_printf(2, 3);

char *try_path(const char *prefix, const char *name, const char *ext);
