/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "rwops_dummy.h"

INLINE SDL_IOStream *SDL_RWWrapReadOnly(SDL_IOStream *src, bool autoclose) {
	return SDL_RWWrapDummy(src, autoclose, true);
}
