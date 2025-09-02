/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "syspath.h"
#include "readonly_wrapper.h"
#include "decompress_wrapper.h"

static void striptrailing(char *p, char **pend) {
	while(*pend > p && strchr(vfs_syspath_separators, *(*pend - 1))) {
		*--*pend = 0;
	}
}

static void stripname(char *p, char **pend) {
	striptrailing(p, pend);
	while(*pend > p && !strchr(vfs_syspath_separators, *(*pend - 1))) {
		*--*pend = 0;
	}
	striptrailing(p, pend);
}

static bool mkdir_with_parents(VFSNode *n, const char *fspath) {
	if(vfs_node_mkdir(n, NULL)) {
		return true;
	}

	char p[strlen(fspath) + 1];
	char *pend = p + sizeof(p) - 1;

	memcpy(p, fspath, sizeof(p));
	stripname(p, &pend);

	if(p == pend) {
		return false;
	}

	VFSNode *pnode;
	if(!(pnode = vfs_syspath_create(p))) {
		return false;
	}

	bool ok = mkdir_with_parents(pnode, p);
	vfs_decref(pnode);
	return ok && vfs_node_mkdir(n, NULL);
}

bool vfs_mount_syspath(const char *mountpoint, const char *fspath, uint flags) {
	VFSNode *rdir;

	if(!(rdir = vfs_syspath_create(fspath))) {
		vfs_set_error("Can't initialize path: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	if((flags & VFS_SYSPATH_MOUNT_MKDIR) && !mkdir_with_parents(rdir, fspath)) {
		vfs_set_error("Can't create directory: %s", vfs_get_error());
		vfs_decref(rdir);
		return false;
	}

	if(flags & VFS_SYSPATH_MOUNT_DECOMPRESSVIEW) {
		VFSNode *rdir_decomp = vfs_decomp_wrap(rdir);
		vfs_decref(rdir);
		rdir = rdir_decomp;
	}

	if(flags & VFS_SYSPATH_MOUNT_READONLY) {
		VFSNode *rdir_ro = vfs_ro_wrap(rdir);
		vfs_decref(rdir);
		rdir = rdir_ro;
	}

	return vfs_mount_or_decref(vfs_root, mountpoint, rdir);
}

char vfs_get_syspath_separator(void) {
	return vfs_syspath_separators[0];
}
