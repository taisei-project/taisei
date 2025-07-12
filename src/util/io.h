/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/arena.h"

#include <SDL3/SDL.h>
#include <stdio.h>

char *SDL_RWgets(SDL_IOStream *rwops, char *buf, size_t bufsize) attr_nonnull_all;
char *SDL_RWgets_realloc(SDL_IOStream *rwops, char **buf, size_t *bufsize) attr_nonnull_all;
char *SDL_RWgets_arena(SDL_IOStream *io, MemArena *arena, size_t *out_buf_size) attr_nonnull(1, 2);
size_t SDL_RWprintf(SDL_IOStream *rwops, const char *fmt, ...) attr_printf(2, 3) attr_nonnull_all;
size_t SDL_RWprintf_arena(SDL_IOStream *rwops, MemArena *scratch, const char *fmt, ...) attr_printf(3, 4) attr_nonnull_all;
size_t SDL_RWvprintf_arena(SDL_IOStream *rwops, MemArena *scratch, const char *fmt, va_list args) attr_nonnull_all;

// This is for the very few legitimate uses for printf/fprintf that shouldn't be replaced with log_*
void tsfprintf(FILE *out, const char *restrict fmt, ...) attr_printf(2, 3);

char *try_path(const char *prefix, const char *name, const char *ext);
