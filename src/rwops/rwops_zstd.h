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

#define RW_ZSTD_LEVEL_DEFAULT 0

SDL_RWops *SDL_RWWrapZstdReader(SDL_RWops *src, bool autoclose);
SDL_RWops *SDL_RWWrapZstdWriter(SDL_RWops *src, int clevel, bool autoclose);

// NOTE: uses inefficient emulation to implement seeking. Source must be seekable as well.
SDL_RWops *SDL_RWWrapZstdReaderSeekable(SDL_RWops *src, int64_t uncompressed_size, bool autoclose);
