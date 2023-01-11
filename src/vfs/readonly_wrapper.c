/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "readonly_wrapper.h"
#include "private.h"
#include "rwops/rwops_ro.h"

VFS_NODE_TYPE(VFSReadOnlyNode, {
	VFSNode *wrapped;
});

#define WRAPPED(n) VFS_NODE_CAST(VFSReadOnlyNode, n)->wrapped

static char *vfs_ro_repr(VFSNode *node) {
	char *wrapped_repr = vfs_node_repr(WRAPPED(node), false);
	char *repr = strjoin("read-only view of ", wrapped_repr, NULL);
	mem_free(wrapped_repr);
	return repr;
}

static void vfs_ro_free(VFSNode *node) {
	vfs_decref(WRAPPED(node));
}

static VFSInfo vfs_ro_query(VFSNode *node) {
	VFSInfo info = vfs_node_query(WRAPPED(node));
	info.is_readonly = true;
	return info;
}

static char* vfs_ro_syspath(VFSNode *node) {
	return vfs_node_syspath(WRAPPED(node));
}

static bool vfs_ro_mount(VFSNode *mountroot, const char *subname, VFSNode *mountee) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static bool vfs_ro_unmount(VFSNode *mountroot, const char *subname) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static VFSNode* vfs_ro_locate(VFSNode *dirnode, const char* path) {
	VFSNode *child = vfs_node_locate(WRAPPED(dirnode), path);

	if(child == NULL) {
		return NULL;
	}

	VFSNode *wrapped_child = vfs_ro_wrap(child);
	vfs_decref(child);

	return wrapped_child;
}

static const char* vfs_ro_iter(VFSNode *dirnode, void **opaque) {
	return vfs_node_iter(WRAPPED(dirnode), opaque);
}

static void vfs_ro_iter_stop(VFSNode *dirnode, void **opaque) {
	vfs_node_iter_stop(WRAPPED(dirnode), opaque);
}

static bool vfs_ro_mkdir(VFSNode *parent, const char *subdir) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static SDL_RWops* vfs_ro_open(VFSNode *filenode, VFSOpenMode mode) {
	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("Read-only filesystem");
		return NULL;
	}

	return SDL_RWWrapReadOnly(vfs_node_open(WRAPPED(filenode), mode), true);
}

VFS_NODE_FUNCS(VFSReadOnlyNode, {
	.repr = vfs_ro_repr,
	.query = vfs_ro_query,
	.free = vfs_ro_free,
	.locate = vfs_ro_locate,
	.syspath = vfs_ro_syspath,
	.iter = vfs_ro_iter,
	.iter_stop = vfs_ro_iter_stop,
	.mkdir = vfs_ro_mkdir,
	.open = vfs_ro_open,
	.mount = vfs_ro_mount,
	.unmount = vfs_ro_unmount,
});

VFSNode *vfs_ro_wrap(VFSNode *base) {
	if(base == NULL) {
		return NULL;
	}

	VFSInfo base_info = vfs_node_query(base);
	vfs_incref(base);

	if(base_info.is_readonly) {
		return base;
	}

	return &VFS_ALLOC(VFSReadOnlyNode, {
		.wrapped = base,
	})->as_generic;
}

bool vfs_make_readonly(const char *path) {
	char buf[strlen(path)+1], *path_parent, *path_subdir;
	vfs_path_normalize(path, buf);

	char npath[strlen(path)+1];
	strcpy(npath, buf);

	if(vfs_query(path).is_readonly) {
		return true;
	}

	vfs_path_split_right(buf, &path_parent, &path_subdir);

	VFSNode *parent = vfs_locate(vfs_root, path_parent);

	if(parent == NULL) {
		return false;
	}

	VFSNode *node = vfs_locate(parent, path_subdir);

	if(node == NULL) {
		vfs_decref(parent);
		return false;
	}

	if(!vfs_node_unmount(parent, path_subdir)) {
		vfs_decref(node);
		vfs_decref(parent);
		return false;
	}

	VFSNode *wrapper = vfs_ro_wrap(node);
	assert(wrapper != NULL);
	vfs_decref(node);

	if(!vfs_node_mount(parent, path_subdir, wrapper)) {
		log_error("Couldn't remount '%s' - VFS left in inconsistent state! Error: %s", npath, vfs_get_error());
		vfs_decref(parent);
		vfs_decref(wrapper);
		return false;
	}

	vfs_decref(parent);
	// vfs_decref(wrapper);
	assert(vfs_query(path).is_readonly);
	return true;
}
