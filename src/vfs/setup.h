/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#pragma once
#include "taisei.h"

#include "public.h"
#include "loadpacks.h"
#include "log.h"

typedef struct VfsSetupFixedPaths {
	const char *res_path;
	const char *storage_path;
	const char *cache_path;
} VfsSetupFixedPaths;

void vfs_setup(CallChain onready);

static inline void vfs_setup_onsync_done(CallChainResult ccr) {
	CallChain *next = ccr.ctx;
	run_call_chain(next, NULL);
	mem_free(next);
}

static inline void vfs_setup_res_syspath(const char *res_path) {
	log_info("Resource path: %s", res_path);

	vfs_create_union_mountpoint("res");

	if(!vfs_mount_syspath("res-dir", res_path, VFS_SYSPATH_MOUNT_READONLY | VFS_SYSPATH_MOUNT_DECOMPRESSVIEW)) {
		log_fatal("Failed to mount '%s': %s", res_path, vfs_get_error());
	}

	vfs_load_packages("res-dir", "res");
	vfs_mount_alias("res", "res-dir");
	vfs_unmount("res-dir");
}

static inline void vfs_setup_storage_syspath(const char *storage_path) {
	log_info("Storage path: %s", storage_path);

	if(!vfs_mount_syspath("storage", storage_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", storage_path, vfs_get_error());
	}

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");
}

static inline void vfs_setup_cache_syspath(const char *cache_path) {
	if(!vfs_mount_syspath("cache", cache_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", cache_path, vfs_get_error());
	}
}

static inline void vfs_setup_fixedpaths(VfsSetupFixedPaths *paths) {
	assume(paths != NULL);
	assume(paths->res_path != NULL);
	assume(paths->storage_path != NULL);
	assume(paths->cache_path != NULL);

	vfs_setup_res_syspath(paths->res_path);
	vfs_setup_storage_syspath(paths->storage_path);
	vfs_setup_cache_syspath(paths->cache_path);
}

#define VFS_SETUP_SYNCING(onsync) \
	void vfs_setup(CallChain next) { \
		vfs_init(); \
		CallChain *cc = memdup(&next, sizeof(next)); \
		vfs_sync(VFS_SYNC_LOAD, CALLCHAIN(onsync, cc)); \
	}
