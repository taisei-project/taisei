/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "private.h"

#include "log.h"
#include "memory/scratch.h"
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

bool vfs_node_syspath(VFSNode *node, StringBuffer *buf) {
	assert(node->funcs != NULL);

	if(node->funcs->syspath == NULL) {
		vfs_set_error("Node doesn't represent a system path");
		return false;
	}

	return node->funcs->syspath(node, buf);
}

void (vfs_node_repr)(VFSNode *node, bool try_syspath, StringBuffer *buf) {
	assert(node->funcs != NULL);
	assert(node->funcs->repr != NULL);

	if(try_syspath && node->funcs->syspath) {
		if(node->funcs->syspath(node, buf)) {
			return;
		}
	}

	VFSInfo i = vfs_node_query(node);

	strbuf_cat(buf, "[");

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
		strbuf_cat(buf, "(");

		for(;;) {
			#define HANDLE(attrib) \
				if(attribs & attrib) { \
					attribs &= ~attrib; \
					strbuf_cat(buf, #attrib); \
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

			strbuf_cat(buf, ", ");
		}

		strbuf_cat(buf, ") ");
	}

	node->funcs->repr(node, buf);

	strbuf_cat(buf, "]");
	return;
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

SDL_IOStream *vfs_node_open(VFSNode *filenode, VFSOpenMode mode) {
	assert(filenode->funcs != NULL);

	if(filenode->funcs->open == NULL) {
		vfs_set_error("Node can't be opened as a file");
		return NULL;
	}

	SDL_IOStream *stream = filenode->funcs->open(filenode, mode);

	if(!stream) {
		return NULL;
	}

	bool wrap = false;
	RWWrapDummyOpts opts = { .autoclose = true };

	if(!(mode & VFS_MODE_WRITE) && !vfs_node_query(filenode).is_readonly) {
		wrap = true;
		opts.readonly = true;
	}

	if(wrap) {
		stream = SDL_RWWrapDummy(stream, &opts);
	}

	return stream;
}

const void *vfs_node_mmap_direct(VFSNode *filenode, size_t *out_size) {
	if(NOT_NULL(filenode->funcs)->mmap == NULL) {
		vfs_set_error("Node can't be mmapped");
		return NULL;
	}

	return filenode->funcs->mmap(filenode, out_size);
}

bool vfs_node_munmap_direct(VFSNode *filenode, const void *data, size_t size) {
	if(NOT_NULL(filenode->funcs)->munmap == NULL) {
		vfs_set_error("Node can't be munmapped");
		return false;
	}

	return filenode->funcs->munmap(filenode, data, size);
}

static VFSMMapTicket _vfs_node_mmap(VFSNode *filenode, const void **addr, size_t *size) {
	VFSMMapTicket ticket = {};
	ticket._internal.addr.as_ptr = (void*)vfs_node_mmap_direct(filenode, &ticket._internal.size);

	*addr = ticket._internal.addr.as_ptr;
	*size = ticket._internal.size;

	return ticket;
}

VFSMMapTicket vfs_node_mmap(VFSNode *filenode, const void **addr, size_t *size, bool allow_fallback) {
	VFSMMapTicket ticket = _vfs_node_mmap(filenode, addr, size);

	if(!allow_fallback || vfs_mmap_ticket_valid(ticket)) {
		return ticket;
	}

	StringBuffer buf = { acquire_scratch_arena() };
	vfs_node_repr(filenode, true, &buf);
	log_warn("%s: Can't memory-map, reading whole file into memory", buf.start);
	release_scratch_arena(buf.arena);

	SDL_IOStream *io = vfs_node_open(filenode, VFS_MODE_READ);

	if(UNLIKELY(!io)) {
		goto fail;
	}

	ticket._internal.addr.as_ptr = SDL_LoadFile_IO(io, &ticket._internal.size, true);

	if(UNLIKELY(!ticket._internal.addr.as_ptr)) {
		goto fail;
	}

	*addr = ticket._internal.addr.as_ptr;
	*size = ticket._internal.size;

	assert((ticket._internal.addr.as_uint & 1) == 0);
	ticket._internal.addr.as_uint |= 1;

	return ticket;

fail:
	*addr = NULL;
	*size = 0;
	return (VFSMMapTicket) {};
}

bool vfs_node_munmap(VFSNode *filenode, VFSMMapTicket ticket) {
	if(UNLIKELY(!vfs_mmap_ticket_valid(ticket))) {
		vfs_set_error("Invalid mmap ticket");
		return false;
	}

	// Is fallback?
	if(ticket._internal.addr.as_uint & 1) {
		ticket._internal.addr.as_uint &= ~1;
		SDL_free(ticket._internal.addr.as_ptr);
		return true;
	}

	if(NOT_NULL(filenode->funcs)->munmap == NULL) {
		vfs_set_error("Node can't be munmapped");
		return false;
	}

	return filenode->funcs->munmap(filenode,
		ticket._internal.addr.as_ptr, ticket._internal.size);
}
