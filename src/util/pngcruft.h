/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <png.h>
#include <SDL3/SDL.h>

void pngutil_init_rwops_read(png_structp png, SDL_IOStream *rwops);
void pngutil_init_rwops_write(png_structp png, SDL_IOStream *rwops);
void pngutil_setup_error_handlers(png_structp png);
png_structp pngutil_create_read_struct(void);
png_structp pngutil_create_write_struct(void);
