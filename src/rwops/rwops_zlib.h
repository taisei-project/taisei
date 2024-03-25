/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

#define RW_DEFLATE_LEVEL_DEFAULT -1

SDL_IOStream *SDL_RWWrapZlibReader(SDL_IOStream *src, size_t bufsize,
				   bool autoclose);
SDL_IOStream *SDL_RWWrapZlibWriter(SDL_IOStream *src, int clevel,
				   size_t bufsize, bool autoclose);

// For raw deflate streams
SDL_IOStream *SDL_RWWrapInflateReader(SDL_IOStream *src, size_t bufsize,
				      bool autoclose);
SDL_IOStream *SDL_RWWrapDeflateWriter(SDL_IOStream *src, int clevel,
				      size_t bufsize, bool autoclose);

// NOTE: uses inefficient emulation to implement seeking. Source must be seekable as well.
SDL_IOStream *SDL_RWWrapZlibReaderSeekable(SDL_IOStream *src,
					   int64_t uncompressed_size,
					   size_t bufsize, bool autoclose);
SDL_IOStream *SDL_RWWrapInflateReaderSeekable(SDL_IOStream *src,
					      int64_t uncompressed_size,
					      size_t bufsize, bool autoclose);
