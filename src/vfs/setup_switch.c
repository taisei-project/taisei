/*
 * This software is licensed under the terms of the MIT-License
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
	const char *program_dir = nxGetProgramDir();
	char *res_path = strfmt("%s/%s", program_dir, TAISEI_BUILDCONF_DATA_PATH);
	char *storage_path = strfmt("%s/storage", program_dir);
	char *cache_path = strfmt("%s/cache", program_dir);

	VfsSetupFixedPaths paths = {
		.res_path = res_path,
		.storage_path = storage_path,
		.cache_path = cache_path,
	};

	vfs_setup_fixedpaths_onsync(ccr, &paths);

	free(res_path);
	free(storage_path);
	free(cache_path);
}

void vfs_setup(CallChain next) {
	vfs_init();
	CallChain *cc = memdup(&next, sizeof(next));
	vfs_sync(VFS_SYNC_LOAD, CALLCHAIN(vfs_setup_onsync, cc));
}
