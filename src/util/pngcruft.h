/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <png.h>
#include <SDL.h>

void png_init_rwops_read(png_structp png, SDL_RWops *rwops);
void png_init_rwops_write(png_structp png, SDL_RWops *rwops);
void png_setup_error_handlers(png_structp png);
