/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "union.h"

#include "dynarray.h"
#include "hashtable.h"

VFS_NODE_TYPE(VFSUnionNode, {
	DYNAMIC_ARRAY(VFSNode*) members;
});

static void vfs_union_free(VFSNode *node) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);
	dynarray_foreach_elem(&unode->members, VFSNode **node, {
		vfs_decref(*node);
	});
	dynarray_free_data(&unode->members);
}

static VFSNode *vfs_union_get_primary(VFSUnionNode *unode) {
	if(UNLIKELY(unode->members.num_elements == 0)) {
		vfs_set_error("The union is empty");
		return NULL;
	}

	return NOT_NULL(dynarray_get(&unode->members, unode->members.num_elements - 1));
}

static VFSNode *vfs_union_locate(VFSNode *node, const char *path) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);

	if(!vfs_union_get_primary(unode)) {
		return NULL;
	}

	VFSNode *dirs[unode->members.num_elements];
	VFSNode **dirs_top = dirs + ARRAY_SIZE(dirs) - 1;
	int num_dirs = 0;

	dynarray_foreach_elem_reversed(&unode->members, VFSNode **member, {
		auto subnode = vfs_locate(*member, path);

		if(!subnode) {
			continue;
		}

		auto subinfo = vfs_node_query(subnode);

		if(!subinfo.exists) {
			vfs_decref(subnode);
			continue;
		}

		if(subinfo.is_dir) {
			dirs_top[-num_dirs] = subnode;
			++num_dirs;
		} else {
			if(num_dirs == 0) {
				return subnode;
			}

			vfs_decref(subnode);
		}
	});

	if(num_dirs == 0) {
		vfs_set_error("No such file or directory: %s", path);
		return NULL;
	}

	if(num_dirs == 1) {
		return dirs_top[0];
	}

	auto subunion = VFS_ALLOC(VFSUnionNode);
	dynarray_set_elements(&subunion->members, num_dirs, &dirs_top[1 - num_dirs]);
	return &subunion->as_generic;
}

typedef struct VFSUnionIterData {
	ht_strset_t visited;
	void *opaque;
	int idx;
} VFSUnionIterData;

static const char *vfs_union_iter(VFSNode *node, void **opaque) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);
	VFSUnionIterData *i = *opaque;
	const char *r = NULL;

	if(!i) {
		i = ALLOC(typeof(*i));
		i->idx = unode->members.num_elements - 1;
		i->opaque = NULL;
		ht_create(&i->visited);
		*opaque = i;
	}

	while(i->idx >= 0) {
		auto n = dynarray_get(&unode->members, i->idx);
		r = vfs_node_iter(n, &i->opaque);

		if(!r) {
			vfs_node_iter_stop(n, &i->opaque);
			i->opaque = NULL;
			i->idx--;
			continue;
		}

		if(ht_lookup(&i->visited, r, NULL)) {
			continue;
		}

		ht_set(&i->visited, r, HT_EMPTY);
		break;
	}

	return r;
}

static void vfs_union_iter_stop(VFSNode *node, void **opaque) {
	VFSUnionIterData *i = *opaque;

	if(i) {
		ht_destroy(&i->visited);
		mem_free(i);
	}

	*opaque = NULL;
}

static VFSInfo vfs_union_query(VFSNode *node) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);
	auto primary = vfs_union_get_primary(unode);

	if(!primary) {
		return VFSINFO_ERROR;
	}

	auto i = vfs_node_query(dynarray_get(&unode->members, unode->members.num_elements - 1));
	// can't trust the primary member here, others might be writable
	i.is_readonly = false;
	return i;
}

static bool vfs_union_mount(VFSNode *node, const char *mountpoint, VFSNode *mountee) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);

	if(mountpoint) {
		vfs_set_error("Attempted to use a named mountpoint '%s' on a union", mountpoint);
		return false;
	}

	auto minfo = vfs_node_query(mountee);

	if(!minfo.is_dir || !minfo.exists) {
		char *r = vfs_node_repr(mountee, true);
		vfs_set_error("Union mountee is not a directory: %s", r);
		mem_free(r);
		return false;
	}

	dynarray_append(&unode->members, mountee);
	assert(vfs_union_get_primary(unode) == mountee);

	return true;
}

static SDL_IOStream *vfs_union_open(VFSNode *node, VFSOpenMode mode) {
	auto primary = vfs_union_get_primary(VFS_NODE_CAST(VFSUnionNode, node));
	return primary ? vfs_node_open(primary, mode) : NULL;
}

static char *vfs_union_repr(VFSNode *node) {
	auto unode = VFS_NODE_CAST(VFSUnionNode, node);
	StringBuffer sb = {};
	strbuf_free(&sb);
	strbuf_cat(&sb, "union: ");

	dynarray_foreach(&unode->members, int idx, VFSNode **node, {
		char *r = vfs_node_repr(*node, false);
		strbuf_cat(&sb, r);
		mem_free(r);

		if(idx < unode->members.num_elements - 1) {
			strbuf_cat(&sb, ", ");
		}
	});

	return sb.start;
}

static char *vfs_union_syspath(VFSNode *node) {
	auto primary = vfs_union_get_primary(VFS_NODE_CAST(VFSUnionNode, node));
	return primary ? vfs_node_syspath(primary) : NULL;
}

static bool vfs_union_mkdir(VFSNode *node, const char *subdir) {
	auto primary = vfs_union_get_primary(VFS_NODE_CAST(VFSUnionNode, node));
	return primary ? vfs_node_mkdir(primary, subdir) : false;
}

VFS_NODE_FUNCS(VFSUnionNode, {
	.repr = vfs_union_repr,
	.query = vfs_union_query,
	.free = vfs_union_free,
	.syspath = vfs_union_syspath,
	.mount = vfs_union_mount,
	.locate = vfs_union_locate,
	.iter = vfs_union_iter,
	.iter_stop = vfs_union_iter_stop,
	.mkdir = vfs_union_mkdir,
	.open = vfs_union_open,
});

VFSNode *vfs_union_create(void) {
	return &VFS_ALLOC(VFSUnionNode)->as_generic;
}

bool vfs_create_union_mountpoint(const char *mountpoint) {
	return vfs_mount_or_decref(vfs_root, mountpoint, vfs_union_create());
}
