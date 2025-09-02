/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "loadpacks.h"

#include "public.h"
#include "error.h"

#include "log.h"
#include "util/stringops.h"

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

void vfs_load_packages(const char *dir, const char *unionmp) {
	// go over the packages in dir in alphabetical order and merge them into unionmp
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
			log_error("VFS error: %s", vfs_get_error());
		}

		mem_free(tmp);
	}

	vfs_dir_list_free(paklist, numpaks);
}
