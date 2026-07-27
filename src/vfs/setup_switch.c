/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#include "setup.h"

#include "memory/scratch.h"
#include "util/strbuf.h"

static void vfs_setup_onsync(CallChainResult ccr) {
	const char *program_dir = nxGetProgramDir();
	StringBuffer buf = { acquire_scratch_arena() };
	VfsSetupFixedPaths paths = {};

	strbuf_printf(&buf, "%s/%s", program_dir, TAISEI_BUILDCONF_DATA_PATH);
	paths.res_path = strbuf_commit(&buf);

	strbuf_printf(&buf, "%s/storage", program_dir);
	paths.storage_path = strbuf_commit(&buf);

	strbuf_printf(&buf, "%s/cache", program_dir);
	paths.cache_path = strbuf_commit(&buf);

	vfs_setup_fixedpaths(&paths);

	release_scratch_arena(buf.arena);

	vfs_setup_onsync_done(ccr);
}

VFS_SETUP_SYNCING(vfs_setup_onsync)
