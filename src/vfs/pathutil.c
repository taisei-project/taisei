/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "private.h"
#include "syspath.h"

char* vfs_path_normalize(const char *path, char *out) {
	const char *p = path;
	char *o = out;
	char *last_sep = out - 1;
	char *path_end = strchr(path, 0);

	while(p < path_end) {
		if(strchr(VFS_PATH_SEPS, *p)) {
			if(o > out && *(o - 1) != VFS_PATH_SEP) {
				last_sep = o;
				*o++ = VFS_PATH_SEP;
			}

			do {
				++p;
			} while(p < path_end && strchr(VFS_PATH_SEPS, *p));
		} else if(*p == '.' && strchr(VFS_PATH_SEPS, *(p + 1))) {
			p += 2;
		} else if(!strncmp(p, "..", 2) && strchr(VFS_PATH_SEPS, *(p + 2))) {
			if(last_sep >= out) {
				do {
					--last_sep;
				} while(*last_sep != VFS_PATH_SEP && last_sep >= out);

				o = last_sep-- + 1;
			}

			p += 3;
		} else {
			*o++ = *p++;
		}
	}

	*o = 0;

	// log_debug("[%s] --> [%s]", path, out);
	return out;
}

char* vfs_path_normalize_alloc(const char *path) {
	return vfs_path_normalize(path, strdup(path));
}

char* vfs_path_normalize_inplace(char *path) {
	char buf[strlen(path)+1];
	strcpy(buf, path);
	vfs_path_normalize(path, buf);
	strcpy(path, buf);
	return path;
}

void vfs_path_split_left(char *path, char **lpath, char **rpath) {
	char *sep;

	while(*path == VFS_PATH_SEP)
		++path;

	*lpath = path;

	if((sep = strchr(path, VFS_PATH_SEP))) {
		*sep = 0;
		*rpath = sep + 1;
	} else {
		*rpath = path + strlen(path);
	}
}

void vfs_path_split_right(char *path, char **lpath, char **rpath) {
	char *sep, *c;
	assert(*path != 0);

	while(*(c = strrchr(path, 0) - 1) == VFS_PATH_SEP)
		*c = 0;

	if((sep = strrchr(path, VFS_PATH_SEP))) {
		*sep = 0;
		*lpath = path;
		*rpath = sep + 1;
	} else {
		*lpath = path + strlen(path);
		*rpath = path;
	}
}

void vfs_path_root_prefix(char *path) {
	if(!strchr(VFS_PATH_SEPS, *path)) {
		memmove(path+1, path, strlen(path)+1);
		*path = VFS_PATH_SEP;
	} else {
		*path = VFS_PATH_SEP;
	}
}

char* vfs_syspath_normalize_inplace(char *path) {
	char buf[strlen(path)+1];
	strcpy(buf, path);
	vfs_syspath_normalize(buf, sizeof(buf), path);
	strcpy(path, buf);
	return path;
}
