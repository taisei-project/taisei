/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "public.h"
#include "setup.h"
#include "util.h"

static void vfs_setup_onsync(CallChainResult ccr) {
	const char *res_path = "/" TAISEI_BUILDCONF_DATA_PATH;
	const char *storage_path = "/persistent/storage";
	const char *cache_path = "/persistent/cache";

	log_info("Resource path: %s", res_path);
	log_info("Storage path: %s", storage_path);
	log_info("Cache path: %s", cache_path);

	if(!vfs_mount_syspath("/res", res_path, VFS_SYSPATH_MOUNT_READONLY)) {
		log_fatal("Failed to mount '%s': %s", res_path, vfs_get_error());
	}

	if(!vfs_mount_syspath("/storage", storage_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", storage_path, vfs_get_error());
	}

	if(!vfs_mount_syspath("/cache", cache_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", cache_path, vfs_get_error());
	}

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");

	CallChain *next = ccr.ctx;
	run_call_chain(next, NULL);
	free(next);
}

void vfs_setup(CallChain next) {
	vfs_init();
	CallChain *cc = memdup(&next, sizeof(next));
	vfs_sync(VFS_SYNC_LOAD, CALLCHAIN(vfs_setup_onsync, cc));
}
