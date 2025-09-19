/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "pathutil.h"

#include "public.h"

char *vfs_path_normalize(const char *path, char *out) {
	const char *p = path;
	char *o = out;
	char *last_sep = out - 1;
	char *path_end = strchr(path, 0);

	#define IS_SEP_OR_NUL(chr) (VFS_IS_PATH_SEPARATOR(chr) || (chr == '\0'))

	while(p < path_end) {
		if(VFS_IS_PATH_SEPARATOR(*p)) {
			if(o > out && !VFS_IS_PATH_SEPARATOR(o[-1])) {
				last_sep = o;
				*o++ = VFS_PATH_SEPARATOR;
			}

			do {
				++p;
			} while(p < path_end && VFS_IS_PATH_SEPARATOR(*p));
		} else if(*p == '.' && IS_SEP_OR_NUL(p[1])) {
			p += 2;
		} else if(p + 1 < path_end && !strncmp(p, "..", 2) && IS_SEP_OR_NUL(p[2])) {
			if(last_sep >= out) {
				do {
					--last_sep;
				} while(last_sep >= out && !VFS_IS_PATH_SEPARATOR(*last_sep));

				o = last_sep-- + 1;
			}

			p += 3;
		} else {
			*o++ = *p++;
		}
	}

	#undef IS_SEP_OR_NUL

	// remove trailing slash
	if(o > out && VFS_IS_PATH_SEPARATOR(o[-1])) {
		--o;
		assert(o == out || !VFS_IS_PATH_SEPARATOR(o[-1]));
	}

	*o = 0;

// 	log_debug("[%s] --> [%s]", path, out);

	return out;
}

char *vfs_path_normalize_alloc(const char *path) {
	return vfs_path_normalize(path, mem_strdup(path));
}

char *vfs_path_normalize_inplace(char *path) {
	char buf[strlen(path)+1];
	strcpy(buf, path);
	vfs_path_normalize(path, buf);
	strcpy(path, buf);
	return path;
}

void vfs_path_split_left(char *path, char **lpath, char **rpath) {
	char *sep;

	while(VFS_IS_PATH_SEPARATOR(*path))
		++path;

	*lpath = path;

	if((sep = strchr(path, VFS_PATH_SEPARATOR))) {
		*sep = 0;
		*rpath = sep + 1;
	} else {
		*rpath = path + strlen(path);
	}
}

void vfs_path_split_right(char *path, char **lpath, char **rpath) {
	char *sep, *c;
	assert(*path != 0);

	while(VFS_IS_PATH_SEPARATOR(*(c = strrchr(path, 0) - 1)))
		*c = 0;

	if((sep = strrchr(path, VFS_PATH_SEPARATOR))) {
		*sep = 0;
		*lpath = path;
		*rpath = sep + 1;
	} else {
		*lpath = path + strlen(path);
		*rpath = path;
	}
}

void vfs_path_resolve_relative(char *buf, size_t bufsize, const char *basepath, const char *relpath) {
	assert(bufsize >= strlen(basepath) + strlen(relpath) + 1);

	if(!*basepath) {
		strcpy(buf, relpath);
		return;
	}

	strcpy(buf, basepath);
	char *end = strrchr(buf, 0);

	while(end > buf) {
		if(VFS_IS_PATH_SEPARATOR(end[-1])) {
			break;
		}

		--end;
	}

	strcpy(end, relpath);
}

void vfs_path_root_prefix(char *path) {
	if(!VFS_IS_PATH_SEPARATOR(*path)) {
		memmove(path+1, path, strlen(path)+1);
		*path = VFS_PATH_SEPARATOR;
	}
}

char *vfs_syspath_normalize_inplace(char *path) {
	char buf[strlen(path)+1];
	strcpy(buf, path);
	vfs_syspath_normalize(buf, sizeof(buf), path);
	strcpy(path, buf);
	return path;
}

char *vfs_syspath_join_alloc(const char *parent, const char *child) {
	char buf[strlen(parent) + strlen(child) + 2];
	vfs_syspath_join(buf, sizeof(buf), parent, child);
	return mem_strdup(buf);
}
