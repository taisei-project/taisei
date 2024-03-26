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

#include "util/sha256.h"

SDL_IOStream *SDL_RWWrapSHA256(SDL_IOStream *src, SHA256State *sha256,
			       bool autoclose);
