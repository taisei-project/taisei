/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

#define RW_DEFLATE_LEVEL_DEFAULT -1

// Pass -1 if uncompressed_size is unknown

// For zlib container streams
SDL_IOStream *SDL_RWWrapZlibReader(SDL_IOStream *src, int64_t uncompressed_size, bool autoclose);
SDL_IOStream *SDL_RWWrapZlibWriter(SDL_IOStream *src, int clevel, bool autoclose);

// For raw deflate streams
SDL_IOStream *SDL_RWWrapInflateReader(SDL_IOStream *src, int64_t uncompressed_size, bool autoclose);
SDL_IOStream *SDL_RWWrapDeflateWriter(SDL_IOStream *src, int clevel, bool autoclose);
