/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "private.h"

#include "dynarray.h"
#include "log.h"

typedef struct VFSDir {
	VFSNode *node;
	void *opaque;
} VFSDir;

bool vfs_mount_alias(const char *dst, const char *src) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char dstbuf[strlen(dst)+1];
	char srcbuf[strlen(src)+1];

	dst = vfs_path_normalize(dst, dstbuf);
	src = vfs_path_normalize(src, srcbuf);

	VFSNode *srcnode = vfs_locate(vfs_root, src);

	if(!srcnode) {
		vfs_set_error("Node '%s' does not exist", src);
		return false;
	}

	return vfs_mount_or_decref(vfs_root, dst, srcnode);
}

bool vfs_unmount(const char *path) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char p[strlen(path)+1], *parent, *subdir;
	path = vfs_path_normalize(path, p);
	vfs_path_split_right(p, &parent, &subdir);
	VFSNode *node = vfs_locate(vfs_root, parent);

	if(node) {
		bool result = vfs_node_unmount(node, subdir);
		vfs_decref(node);
		return result;
	}

	vfs_set_error("Mountpoint root '%s' doesn't exist", parent);
	return false;
}

SDL_IOStream* vfs_open(const char *path, VFSOpenMode mode) {
	if(UNLIKELY(!vfs_initialized())) {
		return NULL;
	}

	SDL_IOStream *rwops = NULL;
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		assert(node->funcs != NULL);

		if(!(rwops = vfs_node_open(node, mode))) {
			vfs_set_error("Can't open '%s': %s", path, vfs_get_error());
		}

		vfs_decref(node);
	} else {
		vfs_set_error("Node '%s' does not exist", path);
	}

	return rwops;
}

VFSInfo vfs_query(const char *path) {
	if(UNLIKELY(!vfs_initialized())) {
		return VFSINFO_ERROR;
	}

	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		// expected to set error on failure
		// note that e.g. a file not existing on a real filesystem
		// is not an error condition. If we can't tell whether it
		// exists or not, that is an error.

		VFSInfo i = vfs_node_query(node);
		vfs_decref(node);
		return i;
	}

	vfs_set_error("Node '%s' does not exist", path);
	return VFSINFO_ERROR;
}

bool vfs_mkdir(const char *path) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);
	bool ok = false;

	if(node) {
		ok = vfs_node_mkdir(node, NULL);
		vfs_decref(node);

		if(ok) {
			return ok;
		}
	}

	char *parent, *subdir;
	vfs_path_split_right(p, &parent, &subdir);
	node = vfs_locate(vfs_root, parent);

	if(node) {
		ok = vfs_node_mkdir(node, subdir);
		vfs_decref(node);
		return ok;
	} else {
		vfs_set_error("Node '%s' does not exist", parent);
		vfs_decref(node);
		return false;
	}
}

void vfs_mkdir_required(const char *path) {
	if(!vfs_mkdir(path)) {
		log_fatal("%s", vfs_get_error());
	}
}

static bool vfs_mkparents_recurse(const char *path) {
	// FIXME this has stupid space complexity and is probably silly in general; optimize if you care

	char *psep = strrchr(path, VFS_PATH_SEPARATOR);

	if(!psep) {
		// parent is root
		return true;
	}

	char p[strlen(path) + 1];
	strcpy(p, path);
	psep += p - path;
	*psep = 0;

	VFSInfo i = vfs_query(p);

	if(i.error) {
		return false;
	}

	if(i.exists) {
		return i.is_dir;
	}

	return vfs_mkparents_recurse(p) && vfs_mkdir(p);
}

bool vfs_mkparents(const char *path) {
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	return vfs_mkparents_recurse(p);
}

char* vfs_repr(const char *path, bool try_syspath) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char buf[strlen(path)+1];
	path = vfs_path_normalize(path, buf);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		char *p = vfs_node_repr(node, try_syspath);
		vfs_decref(node);
		return p;
	}

	vfs_set_error("Node '%s' does not exist", path);
	return NULL;
}

bool vfs_print_tree(SDL_IOStream *dest, const char *path) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char p[strlen(path)+3], *trail;
	vfs_path_normalize(path, p);

	while(*p && *(trail = strchr(p, 0) - 1) == '/') {
		*trail = 0;
	}

	VFSNode *node = vfs_locate(vfs_root, p);

	if(!node) {
		vfs_set_error("Node '%s' does not exist", path);
		return false;
	}

	if(*p) {
		vfs_path_root_prefix(p);
	}

	vfs_print_tree_recurse(dest, node, p, "");
	vfs_decref(node);
	return true;
}

VFSDir* vfs_dir_open(const char *path) {
	if(UNLIKELY(!vfs_initialized())) {
		return false;
	}

	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		if(node->funcs->iter && vfs_node_query(node).is_dir) {
			return ALLOC(VFSDir, { .node = node });
		}

		vfs_set_error("Node '%s' is not a directory", path);
		vfs_decref(node);
	} else {
		vfs_set_error("Node '%s' does not exist", path);
	}

	return NULL;
}

void vfs_dir_close(VFSDir *dir) {
	if(dir) {
		vfs_node_iter_stop(dir->node, &dir->opaque);
		vfs_decref(dir->node);
		mem_free(dir);
	}
}

const char* vfs_dir_read(VFSDir *dir) {
	if(UNLIKELY(!vfs_initialized())) {
		return NULL;
	}

	return vfs_node_iter(dir->node, &dir->opaque);
}

char** vfs_dir_list_sorted(const char *path, size_t *out_size, int (*compare)(const void*, const void*), bool (*filter)(const char*)) {
	VFSDir *dir = vfs_dir_open(path);

	if(!dir) {
		return NULL;
	}

	DYNAMIC_ARRAY(char*) results = { 0 };
	dynarray_ensure_capacity(&results, 2);

	for(const char *e; (e = vfs_dir_read(dir));) {
		if(filter && !filter(e)) {
			continue;
		}

		dynarray_append(&results, mem_strdup(e));
	}

	vfs_dir_close(dir);

	if(results.num_elements) {
		dynarray_qsort(&results, compare);
	}

	*out_size = results.num_elements;
	return results.data;
}

void vfs_dir_list_free(char **list, size_t size) {
	if(!list) {
		return;
	}

	for(size_t i = 0; i < size; ++i) {
		mem_free(list[i]);
	}

	mem_free(list);
}

int vfs_dir_list_order_ascending(const void *a, const void *b) {
	return strcmp(*(char**)a, *(char**)b);
}

int vfs_dir_list_order_descending(const void *a, const void *b) {
	return strcmp(*(char**)b, *(char**)a);
}

void* vfs_dir_walk(const char *path, void* (*visit)(const char *path, void *arg), void *arg) {
	char npath[strlen(path) + 1];
	vfs_path_normalize(path, npath);

	VFSDir *dir = vfs_dir_open(npath);
	void *result = NULL;

	if(!dir) {
		return result;
	}

	for(const char *e; (e = vfs_dir_read(dir));) {
		char fullpath[strlen(npath) + strlen(e) + 2];
		snprintf(fullpath, sizeof(fullpath), "%s%c%s", npath, VFS_PATH_SEPARATOR, e);

		if((result = visit(fullpath, arg))) {
			return result;
		}

		if(!vfs_query(fullpath).is_dir) {
			continue;
		}

		if((result = vfs_dir_walk(fullpath, visit, arg))) {
			return result;
		}
	}

	vfs_dir_close(dir);
	return result;
}

bool vfs_initialized(void) {
	return vfs_root != NULL;
}
