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
	const char* res_path;
	const char* storage_path;
	const char* cache_path;
} VfsSetupFixedPaths;

void vfs_setup(CallChain onready);

static inline void vfs_setup_fixedpaths_onsync(CallChainResult ccr, VfsSetupFixedPaths* paths) {
	assume(paths != NULL);
	assume(paths->res_path != NULL);
	assume(paths->storage_path != NULL);
	assume(paths->cache_path != NULL);

	log_info("Resource path: %s", paths->res_path);
	log_info("Storage path: %s", paths->storage_path);
	log_info("Cache path: %s", paths->cache_path);

	vfs_create_union_mountpoint("/res");

	if(!vfs_mount_syspath("/res-dir", paths->res_path, VFS_SYSPATH_MOUNT_READONLY | VFS_SYSPATH_MOUNT_DECOMPRESSVIEW)) {
		log_fatal("Failed to mount '%s': %s", paths->res_path, vfs_get_error());
	}

	if(!vfs_mount_syspath("/storage", paths->storage_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", paths->storage_path, vfs_get_error());
	}

	if(!vfs_mount_syspath("/cache", paths->cache_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", paths->cache_path, vfs_get_error());
	}

	vfs_load_packages("/res-dir", "/res");
	vfs_mount_alias("/res", "/res-dir");
	vfs_unmount("/res-dir");

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");

	CallChain *next = ccr.ctx;
	run_call_chain(next, NULL);
	free(next);
}
