/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "zipfile.h"

bool vfs_mount_zipfile(const char *mountpoint, const char *zippath) {
	char p[strlen(zippath)+1];
	zippath = vfs_path_normalize(zippath, p);
	VFSNode *node = vfs_locate(vfs_root, zippath);

	if(!node) {
		vfs_set_error("Node '%s' does not exist", zippath);
		return false;
	}

	VFSNode *znode;

	if(!(znode = vfs_zipfile_create(node))) {
		vfs_decref(node);
		return false;
	}

	if(!vfs_mount(vfs_root, mountpoint, znode)) {
		vfs_decref(znode);
		vfs_decref(node);
		return false;
	}

	return true;
}
