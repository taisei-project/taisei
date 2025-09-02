/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "private.h"
#include "vdir.h"

#include "list.h"
#include "log.h"
#include "util/io.h"
#include "util/stringops.h"

VFSNode *vfs_root = NULL;

typedef struct vfs_tls_s {
	char *error_str;
} vfs_tls_t;

typedef struct vfs_shutdownhook_t {
	LIST_INTERFACE(struct vfs_shutdownhook_t);
	VFSShutdownHandler func;
	void *arg;
} vfs_shutdownhook_t;

static SDL_TLSID vfs_tls_id;
static vfs_tls_t *vfs_tls_fallback;
static vfs_shutdownhook_t *shutdown_hooks;

static void vfs_free(VFSNode *node);

static void vfs_tls_free(void *vtls) {
	if(vtls) {
		vfs_tls_t *tls = vtls;
		mem_free(tls->error_str);
		mem_free(tls);
	}
}

static vfs_tls_t* vfs_tls_get(void) {
	vfs_tls_t *tls = SDL_GetTLS(&vfs_tls_id) ?: vfs_tls_fallback;

	if(tls) {
		return tls;
	}

	tls = ALLOC(vfs_tls_t);

	if(!SDL_SetTLS(&vfs_tls_id, tls, vfs_tls_free)) {
		log_warn("SDL_SetTLS(): failed: %s", SDL_GetError());
		vfs_tls_fallback = tls;
	}

	return tls;
}

void vfs_init(void) {
	vfs_root = vfs_vdir_create();
}

static void* call_shutdown_hook(List **vlist, List *vhook, void *arg) {
	vfs_shutdownhook_t *hook = (vfs_shutdownhook_t*)vhook;
	hook->func(hook->arg);
	mem_free(list_unlink(vlist, vhook));
	return NULL;
}

void vfs_shutdown(void) {
	vfs_sync(VFS_SYNC_STORE, NO_CALLCHAIN);

	list_foreach(&shutdown_hooks, call_shutdown_hook, NULL);

	vfs_decref(vfs_root);
	vfs_tls_free(vfs_tls_fallback);

	vfs_root = NULL;
	vfs_tls_fallback = NULL;
}

void vfs_hook_on_shutdown(VFSShutdownHandler func, void *arg) {
	list_append(&shutdown_hooks, ALLOC(vfs_shutdownhook_t, {
		.func = func,
		.arg = arg,
	}));
}

static void vfs_free(VFSNode *node) {
	if(!node) {
		return;
	}

	if(node->funcs && node->funcs->free) {
		node->funcs->free(node);
	}

	mem_free(node);
}

void (vfs_incref)(VFSNode *node) {
	SDL_AtomicIncRef(&node->refcount);
}

bool (vfs_decref)(VFSNode *node) {
	if(!node) {
		return true;
	}

	if(SDL_AtomicDecRef(&node->refcount)) {
		vfs_free(node);
		return true;
	}

	return false;
}

VFSNode *(vfs_locate)(VFSNode *root, const char *path) {
	if(!*path) {
		vfs_incref(root);
		return root;
	}

	return vfs_node_locate(root, path);
}

bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree) {
	VFSNode *mpnode;
	char buf[2][strlen(mountpoint)+1];
	char *mpbase, *mpname;
	bool result = false;

	mountpoint = vfs_path_normalize(mountpoint, buf[0]);
	strcpy(buf[1], buf[0]);
	vfs_path_split_right(buf[1], &mpbase, &mpname);

	if((mpnode = vfs_locate(root, mountpoint))) {
		// mountpoint already exists - try to merge with the target node

		result = vfs_node_mount(mpnode, NULL, subtree);

		if(!result) {
			vfs_set_error("Mountpoint '%s' already exists, merging failed: %s", mountpoint, vfs_get_error());
		}

		vfs_decref(mpnode);
		return result;
	}

	if((mpnode = vfs_locate(root, mpbase))) {
		// try to become a subnode of parent (conventional mount)

		result = vfs_node_mount(mpnode, mpname, subtree);

		if(!result) {
			vfs_set_error("Can't create mountpoint '%s' in '%s': %s", mountpoint, mpbase, vfs_get_error());
		}

		vfs_decref(mpnode);
	}

	// error set by vfs_locate
	return result;
}

bool vfs_mount_or_decref(VFSNode *root, const char *mountpoint, VFSNode *subtree) {
	if(!vfs_mount(root, mountpoint, subtree)) {
		vfs_decref(subtree);
		return false;
	}

	return true;
}

void vfs_print_tree_recurse(SDL_IOStream *dest, VFSNode *root, char *prefix,
			    const char *name) {
	void *o = NULL;
	bool is_dir = vfs_node_query(root).is_dir;
	char *newprefix = strfmt("%s%s%s", prefix, name, is_dir ? VFS_PATH_SEPARATOR_STR : "");
	char *r;

	r = vfs_node_repr(root, false);
	SDL_RWprintf(dest, "%s = %s\n", newprefix, r);
	mem_free(r);

	if(!is_dir) {
		mem_free(newprefix);
		return;
	}

	for(const char *n; (n = vfs_node_iter(root, &o));) {
		VFSNode *node = vfs_locate(root, n);
		if(node) {
			vfs_print_tree_recurse(dest, node, newprefix, n);
			vfs_decref(node);
		}
	}

	vfs_node_iter_stop(root, &o);
	mem_free(newprefix);
}

const char* vfs_get_error(void) {
	if(UNLIKELY(!vfs_initialized())) {
		return "VFS is not initialized";
	}

	vfs_tls_t *tls = vfs_tls_get();
	return tls->error_str ? tls->error_str : "No error";
}

void (vfs_set_error)(char *fmt, ...) {
	vfs_tls_t *tls = vfs_tls_get();
	va_list args;
	va_start(args, fmt);
	char *err = vstrfmt(fmt, args);
	mem_free(tls->error_str);
	tls->error_str = err;
	va_end(args);

	// log_debug("%s", tls->error_str);
}

void vfs_set_error_from_sdl(void) {
	vfs_set_error("SDL error: %s", SDL_GetError());
}
