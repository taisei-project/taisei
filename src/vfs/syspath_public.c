/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "syspath.h"
#include "readonly_wrapper.h"

bool vfs_mount_syspath(const char *mountpoint, const char *fspath, uint flags) {
	VFSNode *rdir = vfs_alloc();

	if(!vfs_syspath_init(rdir, fspath)) {
		vfs_set_error("Can't initialize path: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	if((flags & VFS_SYSPATH_MOUNT_MKDIR) && !vfs_node_mkdir(rdir, NULL)) {
		vfs_set_error("Can't create directory: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	if(flags & VFS_SYSPATH_MOUNT_READONLY) {
		VFSNode *rdir_ro = vfs_ro_wrap(rdir);
		vfs_decref(rdir);
		rdir = rdir_ro;
	}

	return vfs_mount_or_decref(vfs_root, mountpoint, rdir);
}
