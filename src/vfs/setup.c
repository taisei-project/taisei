/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "build_config.h"
#include "public.h"
#include "setup.h"
#include "error.h"

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
		*cache = strfmt("%s%ccache", *storage, vfs_get_syspath_separator());
	}
}

static bool vfs_mount_pkgdir(const char *dst, const char *src) {
	VFSInfo stat = vfs_query(src);

	if(stat.error) {
		return false;
	}

	if(!stat.exists || !stat.is_dir) {
		vfs_set_error("Not a directory");
		return false;
	}

	return vfs_mount_alias(dst, src);
}

static struct pkg_loader_t {
	const char *const ext;
	bool (*mount)(const char *mp, const char *arg);
} pkg_loaders[] = {
	{ ".zip",       vfs_mount_zipfile },
	{ ".pkgdir",    vfs_mount_pkgdir  },
	{ NULL },
};

static struct pkg_loader_t* find_loader(const char *str) {
	char buf[strlen(str) + 1];
	memset(buf, 0, sizeof(buf));

	for(char *p = buf; *str; ++p, ++str) {
		*p = tolower(*str);
	}

	for(struct pkg_loader_t *l = pkg_loaders; l->ext; ++l) {
		if(strendswith(buf, l->ext)) {
			return l;
		}
	}

	return NULL;
}

static void load_packages(const char *dir, const char *unionmp) {
	// go over the packages in dir in alphapetical order and merge them into unionmp
	// e.g. files in 00-aaa.zip will be shadowed by files in 99-zzz.zip

	size_t numpaks = 0;
	char **paklist = vfs_dir_list_sorted(dir, &numpaks, vfs_dir_list_order_ascending, NULL);

	if(!paklist) {
		log_fatal("VFS error: %s", vfs_get_error());
	}

	for(size_t i = 0; i < numpaks; ++i) {
		const char *entry = paklist[i];
		struct pkg_loader_t *loader = find_loader(entry);

		if(loader == NULL) {
			continue;
		}

		log_info("Adding package: %s", entry);
		assert(loader->mount != NULL);

		char *tmp = strfmt("%s/%s", dir, entry);

		if(!loader->mount(unionmp, tmp)) {
			log_warn("VFS error: %s", vfs_get_error());
		}

		free(tmp);
	}

	vfs_dir_list_free(paklist, numpaks);
}

void vfs_setup(bool silent) {
	char *res_path, *storage_path, *cache_path;
	get_core_paths(&res_path, &storage_path, &cache_path);

	char *local_res_path = strfmt("%s/resources", storage_path);

	if(!silent) {
		log_info("Resource path: %s", res_path);
		log_info("Storage path: %s", storage_path);
		log_info("Local resource path: %s", local_res_path);
		log_info("Cache path: %s", cache_path);
	}

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
				log_warn("'%s' is not a directory", mp->syspath);
				vfs_unmount("tmp");
				continue;
			}

			// load all packages from this directory into the respkgs union
			load_packages("tmp", "respkgs");

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
}
