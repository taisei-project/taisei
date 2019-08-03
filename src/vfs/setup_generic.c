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

static char* get_default_res_path(void) {
	char *res;

#ifdef TAISEI_BUILDCONF_RELATIVE_DATA_PATH
	res = SDL_GetBasePath();
	strappend(&res, TAISEI_BUILDCONF_DATA_PATH);
#else
	res = strdup(TAISEI_BUILDCONF_DATA_PATH);
#endif

	return res;
}

static void get_core_paths(char **res, char **storage, char **cache) {
	if(*(*res = (char*)env_get("TAISEI_RES_PATH", ""))) {
		*res = strdup(*res);
	} else {
		*res = get_default_res_path();
	}

	if(*(*storage = (char*)env_get("TAISEI_STORAGE_PATH", ""))) {
		*storage = strdup(*storage);
	} else {
		*storage = SDL_GetPrefPath("", "taisei");
	}

	if(*(*cache = (char*)env_get("TAISEI_CACHE_PATH", ""))) {
		*cache = strdup(*cache);
	} else {
		// TODO: follow XDG on linux
		if(*storage != NULL) {
			*cache = strfmt("%s%ccache", *storage, vfs_get_syspath_separator());
		} else {
			*cache = NULL;
		}
	}

	if(*res)     vfs_syspath_normalize_inplace(*res);
	if(*storage) vfs_syspath_normalize_inplace(*storage);
	if(*cache)   vfs_syspath_normalize_inplace(*cache);
}

// NOTE: For simplicity, we will assume that vfs_sync is not needed in this backend.

void vfs_setup(CallChain next) {
	char *res_path, *storage_path, *cache_path;
	get_core_paths(&res_path, &storage_path, &cache_path);

	char *local_res_path = strfmt("%s%cresources", storage_path, vfs_get_syspath_separator());
	vfs_syspath_normalize_inplace(local_res_path);

	log_info("Resource path: %s", res_path);
	log_info("Storage path: %s", storage_path);
	log_info("Local resource path: %s", local_res_path);
	log_info("Cache path: %s", cache_path);

	struct mpoint_t {
		const char *dest;    const char *syspath; bool loadpaks; uint flags;
	} mpts[] = {
		// per-user directory, where configs, replays, screenshots, etc. get stored
		{ "storage",         storage_path,        false,         VFS_SYSPATH_MOUNT_MKDIR },

		// system-wide directory, contains all of the game assets
		{ "resdirs",         res_path,            true,          VFS_SYSPATH_MOUNT_READONLY },

		// subpath of storage, files here override the global assets
		{ "resdirs",         local_res_path,      true,          VFS_SYSPATH_MOUNT_MKDIR | VFS_SYSPATH_MOUNT_READONLY },

		// per-user directory, to contain various cached resources to speed up loading times
		{ "cache",           cache_path,          false,         VFS_SYSPATH_MOUNT_MKDIR },

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
			log_fatal("Failed to mount '%s': %s", mp->syspath, vfs_get_error());
		}
	}

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");

	free(local_res_path);
	free(res_path);
	free(storage_path);
	free(cache_path);

	// set up the final "res" union and get rid of the temporaries

	vfs_mount_alias("res", "respkgs");
	vfs_mount_alias("res", "resdirs");
	// vfs_make_readonly("res");

	vfs_unmount("resdirs");
	vfs_unmount("respkgs");

	run_call_chain(&next, NULL);
}
