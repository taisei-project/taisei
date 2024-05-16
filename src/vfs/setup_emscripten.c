/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#include "setup.h"
#include "emscripten_fetch_public.h"
#include "decompress_wrapper_public.h"

static void vfs_setup_onsync(CallChainResult ccr) {
	vfs_setup_storage_syspath("/persistent/storage");
	vfs_setup_cache_syspath("/persistent/cache");

	vfs_create_union_mountpoint("res");
	attr_unused bool mount_ok = vfs_mount_fetchfs("res-dir");
	assert(mount_ok);
	vfs_make_decompress_view("res-dir");
	vfs_load_packages("res-dir", "res");
	vfs_mount_alias("res", "res-dir");
	vfs_unmount("res-dir");

	vfs_setup_onsync_done(ccr);
}

VFS_SETUP_SYNCING(vfs_setup_onsync)
