/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "union.h"

bool vfs_create_union_mountpoint(const char *mountpoint) {
	VFSNode *unode = vfs_alloc();
	vfs_union_init(unode);
	return vfs_mount_or_decref(vfs_root, mountpoint, unode);
}
