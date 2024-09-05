/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "vdir.h"

#include "hashtable.h"

VFS_NODE_TYPE(VFSVDirNode, {
	ht_str2ptr_t table;
});

static void vfs_vdir_attach_node(VFSNode *node, const char *name, VFSNode *subnode) {
	auto vdir = VFS_NODE_CAST(VFSVDirNode, node);
	VFSNode *oldnode = ht_get(&vdir->table, name, NULL);

	if(oldnode) {
		vfs_decref(oldnode);
	}

	ht_set(&vdir->table, name, subnode);
}

static VFSNode *vfs_vdir_locate(VFSNode *node, const char *path) {
	auto vdir = VFS_NODE_CAST(VFSVDirNode, node);
	VFSNode *subnode;
	char mutpath[strlen(path)+1];
	char *primpath, *subpath;

	strcpy(mutpath, path);
	vfs_path_split_left(mutpath, &primpath, &subpath);

	if((subnode = ht_get(&vdir->table, primpath, NULL))) {
		return vfs_locate(subnode, subpath);
	}

	return NULL;
}

static const char *vfs_vdir_iter(VFSNode *node, void **opaque) {
	auto vdir = VFS_NODE_CAST(VFSVDirNode, node);
	ht_str2ptr_iter_t *iter = *opaque;

	if(!iter) {
		iter = *opaque = ALLOC(typeof(*iter));
		ht_iter_begin(&vdir->table, iter);
	} else {
		ht_iter_next(iter);
	}

	return iter->has_data ? iter->key : NULL;
}

static void vfs_vdir_iter_stop(VFSNode *node, void **opaque) {
	if(*opaque) {
		ht_iter_end((ht_str2ptr_iter_t*)*opaque);
		mem_free(*opaque);
		*opaque = NULL;
	}
}

static VFSInfo vfs_vdir_query(VFSNode *node) {
	return (VFSInfo) {
		.exists = true,
		.is_dir = true,
	};
}

static void vfs_vdir_free(VFSNode *node) {
	auto vdir = VFS_NODE_CAST(VFSVDirNode, node);
	ht_str2ptr_t *ht = &vdir->table;
	ht_str2ptr_iter_t iter;

	ht_iter_begin(ht, &iter);

	for(; iter.has_data; ht_iter_next(&iter)) {
		vfs_decref((VFSNode*)iter.value);
	}

	ht_iter_end(&iter);
	ht_destroy(ht);
}

static bool vfs_vdir_mount(VFSNode *vdir, const char *mountpoint, VFSNode *subtree) {
	if(!mountpoint) {
		vfs_set_error("Virtual directories don't support merging");
		return false;
	}

	vfs_vdir_attach_node(vdir, mountpoint, subtree);
	return true;
}

static bool vfs_vdir_unmount(VFSNode *node, const char *mountpoint) {
	auto vdir = VFS_NODE_CAST(VFSVDirNode, node);
	VFSNode *mountee;

	if(!(mountee = ht_get(&vdir->table, mountpoint, NULL))) {
		vfs_set_error("Mountpoint '%s' doesn't exist", mountpoint);
		return false;
	}

	ht_unset(&vdir->table, mountpoint);
	vfs_decref(mountee);
	return true;
}

static bool vfs_vdir_mkdir(VFSNode *node, const char *subdir) {
	return vfs_vdir_mount(node, NOT_NULL(subdir), vfs_vdir_create());
}

static char *vfs_vdir_repr(VFSNode *node) {
	return mem_strdup("virtual directory");
}

VFS_NODE_FUNCS(VFSVDirNode, {
	.repr = vfs_vdir_repr,
	.query = vfs_vdir_query,
	.free = vfs_vdir_free,
	.mount = vfs_vdir_mount,
	.unmount = vfs_vdir_unmount,
	.locate = vfs_vdir_locate,
	.iter = vfs_vdir_iter,
	.iter_stop = vfs_vdir_iter_stop,
	.mkdir = vfs_vdir_mkdir,
});

VFSNode *vfs_vdir_create(void) {
	auto vdir = VFS_ALLOC(VFSVDirNode);
	ht_create(&vdir->table);
	return &vdir->as_generic;
}
