/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "public.h"
#include "setup.h"
#include "error.h"
#include "util.h"
#include "loadpacks.h"
#include "platform_paths/platform_paths.h"

// NOTE: For simplicity, we will assume that vfs_sync is not needed in this backend.

void vfs_setup(CallChain next) {
	const char *res_path = env_get_string_nonempty("TAISEI_RES_PATH", vfs_platformpath_resroot());
	const char *storage_path = env_get_string_nonempty("TAISEI_STORAGE_PATH", vfs_platformpath_storage());
	const char *cache_path = env_get_string_nonempty("TAISEI_CACHE_PATH", vfs_platformpath_cache());
	char *cache_path_allocated = NULL;
	char *local_res_path = NULL;

	if(storage_path) {
		if(!cache_path) {
			cache_path = cache_path_allocated = vfs_syspath_join_alloc(storage_path, "cache");
		}

		local_res_path = vfs_syspath_join_alloc(storage_path, "resources");
	}

	log_info("Resource path: %s", res_path ? res_path : "(NONE)");
	log_info("Storage path: %s", storage_path ? storage_path : "(NONE)");
	log_info("Local resource path: %s", local_res_path ? local_res_path : "(NONE)");
	log_info("Cache path: %s", cache_path ? cache_path : "(NONE)");

	if(storage_path == NULL) {
		log_warn("No persistent storage directory; your progress will not be saved!");
	}

	struct mpoint_t {
		const char *dest; const char *syspath; bool loadpaks; bool required; uint flags;
	} mpts[] = {
		// per-user directory, where configs, replays, screenshots, etc. get stored
		{ "storage",      storage_path,        false,         false,         VFS_SYSPATH_MOUNT_MKDIR },

		// system-wide directory, contains all of the game assets
		{ "resdirs",      res_path,            true,          true,          VFS_SYSPATH_MOUNT_READONLY },

		// subpath of storage, files here override the global assets
		{ "resdirs",      local_res_path,      true,          false,         VFS_SYSPATH_MOUNT_MKDIR | VFS_SYSPATH_MOUNT_READONLY },

		// per-user directory, to contain various cached resources to speed up loading times
		{ "cache",        cache_path,          false,         false,         VFS_SYSPATH_MOUNT_MKDIR },

		{NULL}
	};

	vfs_init();

	// temporary union of the "real" directories
	vfs_create_union_mountpoint("resdirs");

	// temporary union of the packages (e.g. zip files)
	vfs_create_union_mountpoint("respkgs");

	// permanent union of respkgs and resdirs
	// this way, files in any of the "real" directories always have priority over anything in packages
	vfs_create_union_mountpoint("res");

	for(struct mpoint_t *mp = mpts; mp->dest; ++mp) {
		if(!mp->syspath) {
			log_warn("Mountpoint '%s' skipped because no path is assigned to it", mp->dest);
			continue;
		}

		if(mp->loadpaks) {
			// mount it to a temporary mountpoint to get a list of packages from this directory
			if(!vfs_mount_syspath("tmp", mp->syspath, mp->flags)) {
				log_fatal("Failed to mount '%s': %s", mp->syspath, vfs_get_error());
			}

			if(!vfs_query("tmp").is_dir) {
				log_error("'%s' is not a directory", mp->syspath);
				vfs_unmount("tmp");
				continue;
			}

			// load all packages from this directory into the respkgs union
			vfs_load_packages("tmp", "respkgs");

			// now mount it to the intended destination, and remove the temporary mountpoint
			vfs_mount_alias(mp->dest, "tmp");
			vfs_unmount("tmp");
		} else if(!vfs_mount_syspath(mp->dest, mp->syspath, mp->flags)) {
			log_custom(mp->required ? LOG_FATAL : LOG_ERROR, "Failed to mount '%s': %s", mp->syspath, vfs_get_error());
		}
	}

	if(!vfs_mkdir("storage/replays")) {
		log_warn("%s", vfs_get_error());
	}

	if(!vfs_mkdir("storage/screenshots")) {
		log_warn("%s", vfs_get_error());
	}

	free(local_res_path);
	free(cache_path_allocated);

	// set up the final "res" union and get rid of the temporaries

	vfs_mount_alias("res", "respkgs");
	vfs_mount_alias("res", "resdirs");
	// vfs_make_readonly("res");

	vfs_unmount("resdirs");
	vfs_unmount("respkgs");

	run_call_chain(&next, NULL);
}
