/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "platform_paths.h"

#include "../syspath_public.h"
#include "log.h"

#include <SDL.h>

const char *vfs_platformpath_resroot(void) {
	static char *cached;

	if(!cached) {
		char *basepath = SDL_GetBasePath();

		if(!basepath) {
			log_sdl_error(LOG_FATAL, "SDL_GetBasePath");
			return NULL;
		}

		cached = vfs_syspath_join_alloc(basepath, TAISEI_BUILDCONF_DATA_PATH);
		SDL_free(basepath);
	}

	return cached;
}
