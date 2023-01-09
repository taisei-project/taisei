/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "union.h"

#define _members_ data1
#define _primary_member_ data2

static bool vfs_union_mount_internal(VFSNode *unode, const char *mountpoint, VFSNode *mountee, VFSInfo info, bool seterror);

static void* vfs_union_delete_callback(List **list, List *elem, void *arg) {
	ListContainer *c = (ListContainer*)elem;
	VFSNode *n = c->data;
	vfs_decref(n);
	mem_free(list_unlink(list, elem));
	return NULL;
}

static void vfs_union_free(VFSNode *node) {
	list_foreach((ListContainer**)&node->_members_, vfs_union_delete_callback, NULL);
}

static VFSNode* vfs_union_locate(VFSNode *node, const char *path) {
	VFSNode *u = vfs_alloc();
	vfs_union_init(u); // uniception!

	VFSInfo prim_info = VFSINFO_ERROR;
	ListContainer *first = node->_members_;
	ListContainer *last = first;
	ListContainer *c;

	for(c = first; c; c = c->next) {
		last = c;
	}

	for(c = last; c; c = c->prev) {
		VFSNode *n = c->data;
		VFSNode *o = vfs_locate(n, path);

		if(o) {
			VFSInfo i = vfs_node_query(o);

			if(vfs_union_mount_internal(u, NULL, o, i, false)) {
				prim_info = i;
			} else {
				vfs_decref(o);
			}
		}
	}

	if(u->_primary_member_) {
		if(!((List*)(u->_members_))->next || !prim_info.is_dir) {
			// the temporary union contains just one member, or doesn't represent a directory
			// in those cases it's just a useless wrapper, so let's just return the primary member directly
			VFSNode *n = u->_primary_member_;

			// incref primary member to keep it alive
			vfs_incref(n);

			// destroy the wrapper, also decrefs n
			vfs_decref(u);

			// no need to incref n, vfs_locate did that for us earlier
			return n;
		}
	} else {
		// all in vain...
		vfs_decref(u);
		u = NULL;
	}

	return u;
}

typedef struct VFSUnionIterData {
	ht_str2int_t visited; // XXX: this may not be the most efficient implementation of a "set" structure...
	ListContainer *current;
	void *opaque;
} VFSUnionIterData;

static const char* vfs_union_iter(VFSNode *node, void **opaque) {
	VFSUnionIterData *i = *opaque;
	const char *r = NULL;

	if(!i) {
		i = ALLOC(typeof(*i));
		i->current = node->_members_;
		i->opaque = NULL;
		ht_create(&i->visited);
		*opaque = i;
	}

	while(i->current) {
		VFSNode *n = (VFSNode*)i->current->data;
		r = vfs_node_iter(n, &i->opaque);

		if(!r) {
			vfs_node_iter_stop(n, &i->opaque);
			i->opaque = NULL;
			i->current = i->current->next;
			continue;
		}

		if(ht_get(&i->visited, r, false)) {
			continue;
		}

		ht_set(&i->visited, r, true);
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
	if(node->_primary_member_) {
		VFSInfo i = vfs_node_query(node->_primary_member_);
		// can't trust the primary member here, others might be writable'
		i.is_readonly = false;
		return i;
	}

	vfs_set_error("Union object has no members");
	return VFSINFO_ERROR;
}

static bool vfs_union_mount_internal(VFSNode *unode, const char *mountpoint, VFSNode *mountee, VFSInfo info, bool seterror) {
	if(!info.exists) {
		if(seterror) {
			char *r = vfs_node_repr(mountee, true);
			vfs_set_error("Mountee doesn't represent a usable resource: %s", r);
			mem_free(r);
		}

		return false;
	}

	if(seterror && !info.is_dir) {
		char *r = vfs_node_repr(mountee, true);
		vfs_set_error("Mountee is not a directory: %s", r);
		mem_free(r);
		return false;
	}

	list_push((ListContainer**)&unode->_members_, list_wrap_container(mountee));
	unode->_primary_member_ = mountee;

	return true;
}

static bool vfs_union_mount(VFSNode *unode, const char *mountpoint, VFSNode *mountee) {
	if(mountpoint) {
		vfs_set_error("Attempted to use a named mountpoint '%s' on a union", mountpoint);
		return false;
	}

	return vfs_union_mount_internal(unode, NULL, mountee, vfs_node_query(mountee), true);
}

static SDL_RWops* vfs_union_open(VFSNode *unode, VFSOpenMode mode) {
	VFSNode *n = unode->_primary_member_;

	if(n) {
		return vfs_node_open(n, mode);
	} else {
		vfs_set_error("Union object has no members");
	}

	return NULL;
}

static char* vfs_union_repr(VFSNode *node) {
	char *mlist = strdup("union: "), *r;

	for(ListContainer *c = node->_members_; c; c = c->next) {
		VFSNode *n = c->data;

		strappend(&mlist, r = vfs_node_repr(n, false));
		mem_free(r);

		if(c->next) {
			strappend(&mlist, ", ");
		}
	}

	return mlist;
}

static char* vfs_union_syspath(VFSNode *node) {
	VFSNode *n = node->_primary_member_;

	if(n) {
		return vfs_node_syspath(n);
	}

	vfs_set_error("Union object has no members");
	return NULL;
}

static bool vfs_union_mkdir(VFSNode *node, const char *subdir) {
	VFSNode *n = node->_primary_member_;

	if(n) {
		return vfs_node_mkdir(n, subdir);
	}

	vfs_set_error("Union object has no members");
	return false;
}

static VFSNodeFuncs vfs_funcs_union = {
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
};

void vfs_union_init(VFSNode *node) {
	node->funcs = &vfs_funcs_union;
	node->data1 = node->data2 = NULL;
}
