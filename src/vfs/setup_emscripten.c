/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#include "taisei.h"

#include "public.h"
#include "setup.h"
#include "util.h"

static void vfs_setup_onsync(CallChainResult ccr) {
	VfsSetupFixedPaths paths = {
		.res_path = "/" TAISEI_BUILDCONF_DATA_PATH,
		.storage_path = "/persistent/storage",
		.cache_path ="/persistent/cache",
	};

	vfs_setup_fixedpaths_onsync(ccr, &paths);
}

void vfs_setup(CallChain next) {
	vfs_init();
	CallChain *cc = memdup(&next, sizeof(next));
	vfs_sync(VFS_SYNC_LOAD, CALLCHAIN(vfs_setup_onsync, cc));
}
