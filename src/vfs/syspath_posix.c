/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "syspath.h"

#define _path_ data1

char *vfs_syspath_separators = "/";

static void vfs_syspath_init_internal(VFSNode *node, char *path);

static void vfs_syspath_free(VFSNode *node) {
	mem_free(node->_path_);
}

static VFSInfo vfs_syspath_query(VFSNode *node) {
	struct stat fstat;
	VFSInfo i = {0};

	if(stat(node->_path_, &fstat) >= 0) {
		i.exists = true;
		i.is_dir = S_ISDIR(fstat.st_mode);
	}

	return i;
}

static SDL_RWops* vfs_syspath_open(VFSNode *node, VFSOpenMode mode) {
	mode &= VFS_MODE_RWMASK;
	SDL_RWops *rwops = SDL_RWFromFile(node->_path_, mode == VFS_MODE_WRITE ? "w" : "r");

	if(!rwops) {
		vfs_set_error_from_sdl();
	}

	return rwops;
}

static VFSNode* vfs_syspath_locate(VFSNode *node, const char *path) {
	VFSNode *n = vfs_alloc();
	vfs_syspath_init_internal(n, strfmt("%s%c%s", (char*)node->_path_, VFS_PATH_SEPARATOR, path));
	return n;
}

static const char* vfs_syspath_iter(VFSNode *node, void **opaque) {
	DIR *dir;
	struct dirent *e;

	if(!*opaque) {
		*opaque = opendir(node->_path_);
	}

	if(!(dir = *opaque)) {
		return NULL;
	}

	do {
		e = readdir(dir);
	} while(e && (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")));

	if(e) {
		return e->d_name;
	}

	closedir(dir);
	*opaque = NULL;
	return NULL;
}

static void vfs_syspath_iter_stop(VFSNode *node, void **opaque) {
	if(*opaque) {
		closedir((DIR*)*opaque);
		*opaque = NULL;
	}
}

static char* vfs_syspath_repr(VFSNode *node) {
	return strfmt("filesystem path (posix): %s", (char*)node->_path_);
}

static char* vfs_syspath_syspath(VFSNode *node) {
	char *p = strdup(node->_path_);
	vfs_syspath_normalize_inplace(p);
	return p;
}

static bool vfs_syspath_mkdir(VFSNode *node, const char *subdir) {
	if(!subdir) {
		subdir = "";
	}

	char *p = strfmt("%s%c%s", (char*)node->_path_, VFS_PATH_SEPARATOR, subdir);
	bool ok = !mkdir(p, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	if(!ok && errno == EEXIST) {
		VFSNode *n = vfs_locate(node, subdir);

		if(n && vfs_node_query(n).is_dir) {
			ok = true;
		} else {
			vfs_set_error("%s already exists, and is not a directory", p);
		}

		vfs_decref(n);
	}

	if(!ok) {
		vfs_set_error("Can't create directory %s (errno: %i)", p, errno);
	}

	mem_free(p);
	return ok;
}

static VFSNodeFuncs vfs_funcs_syspath = {
	.repr = vfs_syspath_repr,
	.query = vfs_syspath_query,
	.free = vfs_syspath_free,
	.locate = vfs_syspath_locate,
	.syspath = vfs_syspath_syspath,
	.iter = vfs_syspath_iter,
	.iter_stop = vfs_syspath_iter_stop,
	.mkdir = vfs_syspath_mkdir,
	.open = vfs_syspath_open,
};

void vfs_syspath_normalize(char *buf, size_t bufsize, const char *path) {
	char *bufptr = buf;
	const char *pathptr = path;
	bool skip = false;

	memset(buf, 0, bufsize);

	while(*pathptr && bufptr < (buf + bufsize - 1)) {
		if(!skip) {
			*bufptr++ = *pathptr;
		}

		skip = (pathptr[0] == '/' && pathptr[1] == '/');
		++pathptr;
	}
}

void vfs_syspath_join(char *buf, size_t bufsize, const char *parent, const char *child) {
	size_t l_parent = strlen(parent);
	size_t l_child = strlen(child);
	char sep = vfs_get_syspath_separator();
	assert(bufsize >= l_parent + l_child + 2);

	if(child[0] == sep) {
		memcpy(buf, child, l_child + 1);
	} else {
		memcpy(buf, parent, l_parent);
		buf += l_parent;

		if(l_parent && parent[l_parent - 1] != sep) {
			*buf++ = sep;
		}

		memcpy(buf, child, l_child + 1);
	}
}

static void vfs_syspath_init_internal(VFSNode *node, char *path) {
	vfs_syspath_normalize_inplace(path);
	node->funcs = &vfs_funcs_syspath;
	node->_path_ = path;
}

bool vfs_syspath_init(VFSNode *node, const char *path) {
	vfs_syspath_init_internal(node, strdup(path));
	return true;
}
