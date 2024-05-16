/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "syspath.h"

VFS_NODE_TYPE(VFSSysPathNode, {
	char *path;
});

char *vfs_syspath_separators = "/";

static VFSNode *vfs_syspath_create_internal(char *path);

static void vfs_syspath_free(VFSNode *node) {
	mem_free(VFS_NODE_CAST(VFSSysPathNode, node)->path);
}

static VFSInfo vfs_syspath_query(VFSNode *node) {
	struct stat fstat;
	VFSInfo i = {0};

	if(stat(VFS_NODE_CAST(VFSSysPathNode, node)->path, &fstat) >= 0) {
		i.exists = true;
		i.is_dir = S_ISDIR(fstat.st_mode);
	}

	return i;
}

static SDL_RWops *vfs_syspath_open(VFSNode *node, VFSOpenMode mode) {
	mode &= VFS_MODE_RWMASK;
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	SDL_RWops *rwops = SDL_RWFromFile(pnode->path, mode == VFS_MODE_WRITE ? "w" : "r");

	if(!rwops) {
		vfs_set_error_from_sdl();
	}

	return rwops;
}

static VFSNode *vfs_syspath_locate(VFSNode *node, const char *path) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	return vfs_syspath_create_internal(strjoin(pnode->path, "/", path, NULL));
}

static const char *vfs_syspath_iter(VFSNode *node, void **opaque) {
	DIR *dir;
	struct dirent *e;

	if(!*opaque) {
		*opaque = opendir(VFS_NODE_CAST(VFSSysPathNode, node)->path);
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

static char *vfs_syspath_repr(VFSNode *node) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	return strfmt("filesystem path (posix): %s", pnode->path);
}

static char *vfs_syspath_syspath(VFSNode *node) {
	char *p = strdup(VFS_NODE_CAST(VFSSysPathNode, node)->path);
	vfs_syspath_normalize_inplace(p);
	return p;
}

static bool vfs_syspath_mkdir(VFSNode *node, const char *subdir) {
	if(!subdir) {
		subdir = "";
	}

	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	char *p = strjoin(pnode->path, VFS_PATH_SEPARATOR_STR, subdir, NULL);
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

VFS_NODE_FUNCS(VFSSysPathNode, {
	.repr = vfs_syspath_repr,
	.query = vfs_syspath_query,
	.free = vfs_syspath_free,
	.locate = vfs_syspath_locate,
	.syspath = vfs_syspath_syspath,
	.iter = vfs_syspath_iter,
	.iter_stop = vfs_syspath_iter_stop,
	.mkdir = vfs_syspath_mkdir,
	.open = vfs_syspath_open,
});

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

static VFSNode *vfs_syspath_create_internal(char *path) {
	vfs_syspath_normalize_inplace(path);
	return &VFS_ALLOC(VFSSysPathNode, {
		.path = path,
	})->as_generic;
}

VFSNode *vfs_syspath_create(const char *path) {
	return vfs_syspath_create_internal(strdup(path));
}
