/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

#define RW_ZSTD_LEVEL_DEFAULT 0

SDL_IOStream *SDL_RWWrapZstdReader(SDL_IOStream *src, bool autoclose);
SDL_IOStream *SDL_RWWrapZstdWriter(SDL_IOStream *src, int clevel,
				   bool autoclose);

// NOTE: uses inefficient emulation to implement seeking. Source must be seekable as well.
SDL_IOStream *SDL_RWWrapZstdReaderSeekable(SDL_IOStream *src,
					   int64_t uncompressed_size,
					   bool autoclose);
