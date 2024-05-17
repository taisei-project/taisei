/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "private.h"
#include "rwops/rwops_ro.h"
#include "util/strbuf.h"

VFSInfo vfs_node_query(VFSNode *node) {
	assert(node->funcs != NULL);
	assert(node->funcs->query != NULL);

	return node->funcs->query(node);
}

VFSNode *vfs_node_locate(VFSNode *root, const char *path) {
	assert(root->funcs != NULL);

	#ifndef NDEBUG
	char buf[strlen(path)+1];
	strcpy(buf, path);
	vfs_path_normalize(path, buf);
	assert(!strcmp(path, buf));
	#endif

	if(root->funcs->locate == NULL) {
		vfs_set_error("Node doesn't support subpaths");
		return NULL;
	}

	return root->funcs->locate(root, path);
}

const char *vfs_node_iter(VFSNode *node, void **opaque) {
	assert(node->funcs != NULL);

	if(node->funcs->iter != NULL) {
		return node->funcs->iter(node, opaque);
	}

	vfs_set_error("Node doesn't support iteration");
	return NULL;
}

void vfs_node_iter_stop(VFSNode *node, void **opaque) {
	assert(node->funcs != NULL);

	if(node->funcs->iter_stop) {
		assert(node->funcs->iter != NULL);
		node->funcs->iter_stop(node, opaque);
	}
}

char *vfs_node_syspath(VFSNode *node) {
	assert(node->funcs != NULL);

	if(node->funcs->syspath == NULL) {
		vfs_set_error("Node doesn't represent a system path");
		return NULL;
	}

	return node->funcs->syspath(node);
}

char *(vfs_node_repr)(VFSNode *node, bool try_syspath) {
	assert(node->funcs != NULL);
	assert(node->funcs->repr != NULL);

	if(try_syspath && node->funcs->syspath) {
		char *r;
		if((r = node->funcs->syspath(node))) {
			return r;
		}
	}

	VFSInfo i = vfs_node_query(node);
	char *o = node->funcs->repr(node);

	StringBuffer buf = {};
	strbuf_cat(&buf, "[");

	enum {
		error = 1 << 0,
		missing = 1 << 2,
		dir = 1 << 3,
		ro = 1 << 4,
	};

	uint attribs =
		(i.error * error) |
		(!i.exists * missing) |
		(i.is_dir * dir) |
		(i.is_readonly * ro);

	if(attribs) {
		strbuf_cat(&buf, "(");

		for(;;) {
			#define HANDLE(attrib) \
				if(attribs & attrib) { \
					attribs &= ~attrib; \
					strbuf_cat(&buf, #attrib); \
					goto next; \
				}

			HANDLE(error)
			HANDLE(missing)
			HANDLE(dir)
			HANDLE(ro)

		next:
			if(!attribs) {
				break;
			}

			strbuf_cat(&buf, ", ");
		}

		strbuf_cat(&buf, ") ");
	}

	strbuf_cat(&buf, o);
	mem_free(o);

	strbuf_cat(&buf, "]");
	return buf.start;
}

bool vfs_node_mount(VFSNode *mountroot, const char *subname, VFSNode *mountee) {
	assert(mountroot->funcs != NULL);

	if(mountroot->funcs->mount == NULL) {
		vfs_set_error("Node doesn't support mounting");
		return false;
	}

	return mountroot->funcs->mount(mountroot, subname, mountee);
}

bool vfs_node_unmount(VFSNode *mountroot, const char *subname) {
	assert(mountroot->funcs != NULL);

	if(mountroot->funcs->unmount == NULL) {
		vfs_set_error("Node doesn't support unmounting");
		return false;
	}

	return mountroot->funcs->unmount(mountroot, subname);
}

bool vfs_node_mkdir(VFSNode *parent, const char *subdir) {
	assert(parent->funcs != NULL);

	if(parent->funcs->mkdir == NULL) {
		vfs_set_error("Node doesn't support directory creation");
		return false;
	}

	return parent->funcs->mkdir(parent, subdir);
}

SDL_RWops *vfs_node_open(VFSNode *filenode, VFSOpenMode mode) {
	assert(filenode->funcs != NULL);

	if(filenode->funcs->open == NULL) {
		vfs_set_error("Node can't be opened as a file");
		return NULL;
	}

	SDL_RWops *stream = filenode->funcs->open(filenode, mode);

	if(!stream) {
		return NULL;
	}

	if(!(mode & VFS_MODE_WRITE) && !vfs_node_query(filenode).is_readonly) {
		stream = SDL_RWWrapReadOnly(stream, true);
	}

	return stream;
}
