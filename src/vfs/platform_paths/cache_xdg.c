/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "platform_paths.h"
#include "../syspath_public.h"
#include "util/env.h"

const char *vfs_platformpath_cache(void) {
	static char *cached;

	if(!cached) {
		const char *xdg_cache = env_get_string_nonempty("XDG_CACHE_HOME", NULL);

		if(xdg_cache) {
			cached = vfs_syspath_join_alloc(xdg_cache, "taisei");
		} else {
			const char *home = env_get_string_nonempty("HOME", NULL);

			if(home) {
				cached = vfs_syspath_join_alloc(home, ".cache/taisei");
			}
		}
	}

	return cached;
}
