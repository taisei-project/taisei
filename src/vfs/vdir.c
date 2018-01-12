/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "vdir.h"

#define _contents_ data1

static void vfs_vdir_attach_node(VFSNode *vdir, const char *name, VFSNode *node) {
	VFSNode *oldnode = hashtable_get_string(vdir->_contents_, name);

	if(oldnode) {
		vfs_decref(oldnode);
	}

	hashtable_set_string(vdir->_contents_, name, node);
}

static VFSNode* vfs_vdir_locate(VFSNode *vdir, const char *path) {
	VFSNode *node;
	char mutpath[strlen(path)+1];
	char *primpath, *subpath;

	strcpy(mutpath, path);
	vfs_path_split_left(mutpath, &primpath, &subpath);

	if((node = hashtable_get_string(vdir->_contents_, mutpath))) {
		return vfs_locate(node, subpath);
	}

	return NULL;
}

static const char* vfs_vdir_iter(VFSNode *vdir, void **opaque) {
	char *ret = NULL;

	if(!*opaque) {
		*opaque = hashtable_iter(vdir->_contents_);
	}

	if(!hashtable_iter_next((HashtableIterator*)*opaque, (void**)&ret, NULL)) {
		*opaque = NULL;
	}

	return ret;
}

static void vfs_vdir_iter_stop(VFSNode *vdir, void **opaque) {
	if(*opaque) {
		free(*opaque);
		*opaque = NULL;
	}
}

static VFSInfo vfs_vdir_query(VFSNode *vdir) {
	return (VFSInfo) {
		.exists = true,
		.is_dir = true,
	};
}

static void vfs_vdir_free(VFSNode *vdir) {
	Hashtable *ht = vdir->_contents_;
	HashtableIterator *i;
	VFSNode *child;

	for(i = hashtable_iter(ht); hashtable_iter_next(i, NULL, (void**)&child);) {
		vfs_decref(child);
	}

	hashtable_free(ht);
}

static bool vfs_vdir_mount(VFSNode *vdir, const char *mountpoint, VFSNode *subtree) {
	if(!mountpoint) {
		vfs_set_error("Virtual directories don't support merging");
		return false;
	}

	vfs_vdir_attach_node(vdir, mountpoint, subtree);
	return true;
}

static bool vfs_vdir_unmount(VFSNode *vdir, const char *mountpoint) {
	VFSNode *mountee;

	if(!(mountee = hashtable_get_string(vdir->_contents_, mountpoint))) {
		vfs_set_error("Mountpoint '%s' doesn't exist", mountpoint);
		return false;
	}

	hashtable_unset_string(vdir->_contents_, mountpoint);
	vfs_decref(mountee);
	return true;
}

static bool vfs_vdir_mkdir(VFSNode *node, const char *subdir) {
	if(!subdir) {
		vfs_set_error("Virtual directory trying to create itself? How did you even get here?");
		return false;
	}

	VFSNode *subnode = vfs_alloc();
	vfs_vdir_init(subnode);
	vfs_vdir_mount(node, subdir, subnode);

	return true;
}

static char* vfs_vdir_repr(VFSNode *node) {
	return strdup("virtual directory");
}

static VFSNodeFuncs vfs_funcs_vdir = {
	.repr = vfs_vdir_repr,
	.query = vfs_vdir_query,
	.free = vfs_vdir_free,
	.mount = vfs_vdir_mount,
	.unmount = vfs_vdir_unmount,
	.locate = vfs_vdir_locate,
	.iter = vfs_vdir_iter,
	.iter_stop = vfs_vdir_iter_stop,
	.mkdir = vfs_vdir_mkdir,
};

void vfs_vdir_init(VFSNode *node) {
	node->funcs = &vfs_funcs_vdir;
	node->_contents_ = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
}
