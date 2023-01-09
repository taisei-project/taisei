/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "vdir.h"

#define VDIR_TABLE(vdir) ((ht_str2ptr_t*)((vdir)->data1))

static void vfs_vdir_attach_node(VFSNode *vdir, const char *name, VFSNode *node) {
	VFSNode *oldnode = ht_get(VDIR_TABLE(vdir), name, NULL);

	if(oldnode) {
		vfs_decref(oldnode);
	}

	ht_set(VDIR_TABLE(vdir), name, node);
}

static VFSNode* vfs_vdir_locate(VFSNode *vdir, const char *path) {
	VFSNode *node;
	char mutpath[strlen(path)+1];
	char *primpath, *subpath;

	strcpy(mutpath, path);
	vfs_path_split_left(mutpath, &primpath, &subpath);

	if((node = ht_get(VDIR_TABLE(vdir), mutpath, NULL))) {
		return vfs_locate(node, subpath);
	}

	return NULL;
}

static const char* vfs_vdir_iter(VFSNode *vdir, void **opaque) {
	ht_str2ptr_iter_t *iter = *opaque;

	if(!iter) {
		iter = *opaque = ALLOC(typeof(*iter));
		ht_iter_begin(VDIR_TABLE(vdir), iter);
	} else {
		ht_iter_next(iter);
	}

	return iter->has_data ? iter->key : NULL;
}

static void vfs_vdir_iter_stop(VFSNode *vdir, void **opaque) {
	if(*opaque) {
		ht_iter_end((ht_str2ptr_iter_t*)*opaque);
		mem_free(*opaque);
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
	ht_str2ptr_t *ht = VDIR_TABLE(vdir);
	ht_str2ptr_iter_t iter;

	ht_iter_begin(ht, &iter);

	for(; iter.has_data; ht_iter_next(&iter)) {
		vfs_decref(iter.value);
	}

	ht_iter_end(&iter);
	ht_destroy(ht);
	mem_free(ht);
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

	if(!(mountee = ht_get(VDIR_TABLE(vdir), mountpoint, NULL))) {
		vfs_set_error("Mountpoint '%s' doesn't exist", mountpoint);
		return false;
	}

	ht_unset(VDIR_TABLE(vdir), mountpoint);
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
	node->data1 = ht_str2ptr_new();
}
