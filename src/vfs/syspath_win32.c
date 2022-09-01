/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <windows.h>
#include <shlwapi.h>
#include <SDL.h>

#include "syspath.h"

#define _path_ data1
#define _wpath_ data2

char *vfs_syspath_separators = "\\/";

// taken from SDL
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UTF-16LE", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR*)(void*)SDL_iconv_string("UTF-16LE", "UTF-8", (char *)(S), SDL_strlen(S)+1)

static bool vfs_syspath_init_internal(VFSNode *node, char *path);

static void vfs_syspath_free(VFSNode *node) {
	free(node->_path_);
	free(node->_wpath_);
}

static void _vfs_set_error_win32(const char *file, int line) {
	DWORD err = GetLastError();
	LPVOID buf = NULL;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&buf,
		0, NULL
	);

	char *errstr = WIN_StringToUTF8(buf);
	LocalFree(buf);
	vfs_set_error("Win32 error %lu: %s [%s:%i]", err, errstr, file, line);
	free(errstr);
}

#define vfs_set_error_win32() _vfs_set_error_win32(_TAISEI_SRC_FILE, __LINE__)

static VFSInfo vfs_syspath_query(VFSNode *node) {
	VFSInfo i = {0};

	if(!PathFileExists(node->_wpath_)) {
		i.exists = false;
		return i;
	}

	DWORD attrib = GetFileAttributes(node->_wpath_);

	if(attrib == INVALID_FILE_ATTRIBUTES) {
		vfs_set_error_win32();
		return VFSINFO_ERROR;
	}

	i.exists = true;
	i.is_dir = (bool)(attrib & FILE_ATTRIBUTE_DIRECTORY);

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
	char buf[strlen(path)+1], *base, *name;
	strcpy(buf, path);
	vfs_path_split_right(buf, &base, &name);

	if(!vfs_syspath_init_internal(n, strfmt("%s%c%s", (char*)node->_path_, '\\', path))) {
		vfs_decref(n);
		return NULL;
	}

	return n;
}

typedef struct VFSWin32IterData {
	HANDLE search_handle;
	char *last_result;
} VFSWin32IterData;

static const char* vfs_syspath_iter(VFSNode *node, void **opaque) {
	VFSWin32IterData *idata;
	WIN32_FIND_DATA fdata;
	HANDLE search_handle;

	if(!*opaque) {
		char *pattern = strjoin(node->_path_, "\\*.*", NULL);
		wchar_t *wpattern = WIN_UTF8ToString(pattern);
		free(pattern);
		search_handle = FindFirstFile(wpattern, &fdata);
		free(wpattern);

		if(search_handle == INVALID_HANDLE_VALUE) {
			vfs_set_error_win32();
			return NULL;
		}

		idata = calloc(1, sizeof(VFSWin32IterData));
		idata->search_handle = search_handle;
		idata->last_result = WIN_StringToUTF8(fdata.cFileName);
		*opaque = idata;

		if(!strcmp(idata->last_result, ".") || !strcmp(idata->last_result, "..")) {
			return vfs_syspath_iter(node, opaque);
		}

		return idata->last_result;
	}

	idata = *opaque;
	free(idata->last_result);
	idata->last_result = NULL;

	if(FindNextFile(idata->search_handle, &fdata)) {
		idata->last_result = WIN_StringToUTF8(fdata.cFileName);

		if(!strcmp(idata->last_result, ".") || !strcmp(idata->last_result, "..")) {
			return vfs_syspath_iter(node, opaque);
		}

		return idata->last_result;
	}

	if(GetLastError() != ERROR_NO_MORE_FILES) {
		vfs_set_error_win32();
	}

	return NULL;
}

static void vfs_syspath_iter_stop(VFSNode *node, void **opaque) {
	VFSWin32IterData *idata = *opaque;

	if(idata) {
		if(!FindClose(idata->search_handle)) {
			vfs_set_error_win32();
		}

		free(idata->last_result);
		free(idata);
	}
}

static char* vfs_syspath_repr(VFSNode *node) {
	return strfmt("filesystem path (win32): %s", (char*)node->_path_);
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

	char *p = strfmt("%s%c%s", (char*)node->_path_, '\\', subdir);
	wchar_t *wp = WIN_UTF8ToString(p);
	bool ok = CreateDirectory(wp, NULL);
	DWORD err = GetLastError();

	if(!ok && err == ERROR_ALREADY_EXISTS) {
		VFSNode *n = vfs_locate(node, subdir);

		if(n && vfs_node_query(n).is_dir) {
			ok = true;
		} else {
			vfs_set_error("%s already exists, and is not a directory", p);
		}

		vfs_decref(n);
	}

	if(!ok) {
		vfs_set_error("Can't create directory %s (win32 error: %lu)", p, err);
	}

	free(p);
	free(wp);
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
	// here we just remove redundant separators and convert forward slashes to back slashes.
	// no need to bother with . and ..

	// paths starting with two separators are a special case however (network shares)
	// and are handled appropriately.

	char *bufptr = buf;
	const char *pathptr = path;
	bool skip = false;

	memset(buf, 0, bufsize);

	while(*pathptr && bufptr < (buf + bufsize - 1)) {
		if(!skip) {
			*bufptr++ = (*pathptr == '/' ? '\\' : *pathptr);
		}

		skip = (
			strchr("/\\", pathptr[0]) &&
			strchr("/\\", pathptr[1]) &&
			(bufptr - buf > 1)
		);

		++pathptr;
	}
}

static bool is_absolute(const char *path, size_t len) {
	// starts with drive letter or path separator?
	return
		(len >= 1 && strchr(vfs_syspath_separators, path[0])) ||
		(len >= 2 && path[1] == ':' && isalpha(path[0]));
}

void vfs_syspath_join(char *buf, size_t bufsize, const char *parent, const char *child) {
	size_t l_parent = strlen(parent);
	size_t l_child = strlen(child);
	char sep = vfs_get_syspath_separator();
	assert(bufsize >= l_parent + l_child + 2);

	if(is_absolute(child, l_child)) {
		memcpy(buf, child, l_child + 1);
	} else {
		memcpy(buf, parent, l_parent);
		buf += l_parent;

		if(l_parent && !strchr(vfs_syspath_separators, parent[l_parent - 1])) {
			*buf++ = sep;
		}

		memcpy(buf, child, l_child + 1);
	}
}

static bool vfs_syspath_validate(char *path) {
	char *c = strchr(path, ':');

	if(c && c == (path + 1) && isalpha(*path)) {
		// drive letters are OK
		c = strchr(c + 1, ':');
	}

	if(c) {
		vfs_set_error("Path '%s' contains forbidden character ':'", path);
		return false;
	}

	return true;
}

static bool vfs_syspath_init_internal(VFSNode *node, char *path) {
	vfs_syspath_normalize_inplace(path);

	if(!vfs_syspath_validate(path)) {
		return false;
	}

	node->funcs = &vfs_funcs_syspath;
	node->_path_ = path;
	node->_wpath_ = WIN_UTF8ToString(path);

	return true;
}

bool vfs_syspath_init(VFSNode *node, const char *path) {
	return vfs_syspath_init_internal(node, strdup(path));
}
