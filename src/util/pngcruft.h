/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <png.h>
#include <SDL.h>

void pngutil_init_rwops_read(png_structp png, SDL_RWops *rwops);
void pngutil_init_rwops_write(png_structp png, SDL_RWops *rwops);
void pngutil_setup_error_handlers(png_structp png);
png_structp pngutil_create_read_struct(void);
png_structp pngutil_create_write_struct(void);
