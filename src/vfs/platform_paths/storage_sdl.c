/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "platform_paths.h"

#include "log.h"

#include <SDL3/SDL.h>

const char *vfs_platformpath_storage(void) {
	static char *cached;

	if(!cached) {
		cached = SDL_GetPrefPath("", "taisei");

		if(!cached) {
			log_sdl_error(LOG_ERROR, "SDL_GetPrefPath");
		}
	}

	return cached;
}
