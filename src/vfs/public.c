/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "private.h"

typedef struct VFSDir {
	VFSNode *node;
	void *opaque;
} VFSDir;

bool vfs_mount_alias(const char *dst, const char *src) {
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
	char p[strlen(path)+1], *parent, *subdir;
	path = vfs_path_normalize(path, p);
	vfs_path_split_right(p, &parent, &subdir);
	VFSNode *node = vfs_locate(vfs_root, parent);

	if(node) {
		bool result;

		if(node->funcs->unmount) {
			result = node->funcs->unmount(node, subdir);
		} else {
			result = false;
			vfs_set_error("Node '%s' doesn't support unmounting", parent);
		}

		vfs_decref(node);
		return result;
	}

	vfs_set_error("Mountpoint root '%s' doesn't exist", parent);
	return false;
}

SDL_RWops* vfs_open(const char *path, VFSOpenMode mode) {
	SDL_RWops *rwops = NULL;
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		assert(node->funcs != NULL);

		if(node->funcs->open) {
			// expected to set error on failure
			rwops = node->funcs->open(node, mode);
		} else {
			vfs_set_error("Node '%s' can't be opened as a file", path);
		}

		vfs_decref(node);
	} else {
		vfs_set_error("Node '%s' does not exist", path);
	}

	return rwops;
}

VFSInfo vfs_query(const char *path) {
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		// expected to set error on failure
		// note that e.g. a file not existing on a real filesystem
		// is not an error condition. If we can't tell whether it
		// exists or not, that is an error.

		VFSInfo i = vfs_query_node(node);
		vfs_decref(node);
		return i;
	}

	vfs_set_error("Node '%s' does not exist", path);
	return VFSINFO_ERROR;
}

bool vfs_mkdir(const char *path) {
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);
	bool ok = false;

	if(node && node->funcs->mkdir) {
		ok = node->funcs->mkdir(node, NULL);
		vfs_decref(node);
		return ok;
	}

	vfs_decref(node);

	char *parent, *subdir;
	vfs_path_split_right(p, &parent, &subdir);
	node = vfs_locate(vfs_root, parent);

	if(node) {
		if(node->funcs->mkdir) {
			ok = node->funcs->mkdir(node, subdir);
			node->funcs->mkdir(node, subdir);
			return ok;
		} else {
			vfs_set_error("Node '%s' does not support creation of subdirectories", parent);
		}
	} else {
		vfs_set_error("Node '%s' does not exist", parent);
	}

	vfs_decref(node);
	return false;
}

void vfs_mkdir_required(const char *path) {
	if(!vfs_mkdir(path)) {
		log_fatal("%s", vfs_get_error());
	}
}

char* vfs_repr(const char *path, bool try_syspath) {
	char buf[strlen(path)+1];
	path = vfs_path_normalize(path, buf);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		char *p = vfs_repr_node(node, try_syspath);
		vfs_decref(node);
		return p;
	}

	vfs_set_error("Node '%s' does not exist", path);
	return NULL;
}

bool vfs_print_tree(SDL_RWops *dest, const char *path) {
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
	char p[strlen(path)+1];
	path = vfs_path_normalize(path, p);
	VFSNode *node = vfs_locate(vfs_root, path);

	if(node) {
		if(node->funcs->iter && vfs_query_node(node).is_dir) {
			VFSDir *d = calloc(1, sizeof(VFSDir));
			d->node = node;
			return d;
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
		vfs_iter_stop(dir->node, &dir->opaque);
		vfs_decref(dir->node);
		free(dir);
	}
}

const char* vfs_dir_read(VFSDir *dir) {
	return vfs_iter(dir->node, &dir->opaque);
}

char** vfs_dir_list_sorted(const char *path, size_t *out_size, int (*compare)(const char**, const char**), bool (*filter)(const char*)) {
	char **results = NULL;
	VFSDir *dir = vfs_dir_open(path);

	assert(out_size != NULL);
	assert(compare != NULL);

	if(!dir) {
		return results;
	}

	size_t real_size = 8;
	results = malloc(sizeof(char*) * real_size);
	*out_size = 0;

	for(const char *e; (e = vfs_dir_read(dir));) {
		if(filter && !filter(e)) {
			continue;
		}

		results[(*out_size)++] = strdup(e);

		if(*out_size >= real_size) {
			real_size *= 2;
			results = realloc(results, sizeof(char*) * real_size);
		}
	}

	vfs_dir_close(dir);

	if(*out_size) {
		qsort(results, *out_size, sizeof(char*), (int (*)(const void*, const void*)) compare);
	}

	return results;
}

void vfs_dir_list_free(char **list, size_t size) {
	if(!list) {
		return;
	}

	for(size_t i = 0; i < size; ++i) {
		free(list[i]);
	}

	free(list);
}

int vfs_dir_list_order_ascending(const char **a, const char **b) {
	return strcmp(*a, *b);
}

int vfs_dir_list_order_descending(const char **a, const char **b) {
	return strcmp(*b, *a);
}
