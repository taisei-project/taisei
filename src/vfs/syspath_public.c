/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "syspath.h"

bool vfs_mount_syspath(const char *mountpoint, const char *fspath, bool mkdir) {
	VFSNode *rdir = vfs_alloc();

	if(!vfs_syspath_init(rdir, fspath)) {
		vfs_set_error("Can't initialize path: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	assert(rdir->funcs);
	assert(rdir->funcs->mkdir);

	if(mkdir && !rdir->funcs->mkdir(rdir, NULL)) {
		vfs_set_error("Can't create directory: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	return vfs_mount_or_decref(vfs_root, mountpoint, rdir);
}
