/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "syspath.h"
#include "util/stringops.h"

#include <windows.h>
#include <shlwapi.h>
#include <SDL.h>

VFS_NODE_TYPE(VFSSysPathNode, {
	char *path;
	wchar_t *wpath;
});

char *vfs_syspath_separators = "\\/";

static char *WIN_StringToUTF8(WCHAR *s) {
	size_t slen = SDL_wcslen(s);
	size_t utf16_size = (slen + 1) * sizeof(WCHAR);
	size_t utf8_size = slen + 1;
	char *temp = SDL_iconv_string("UTF-8", "UTF-16LE", (char*)s, utf16_size);
	char *buf = memdup(temp, utf8_size);
	SDL_free(temp);
	return buf;
}

static WCHAR *WIN_UTF8ToString(char *s) {
	size_t slen = strlen(s);
	size_t utf16_size = (slen + 1) * sizeof(WCHAR);
	size_t utf8_size = slen + 1;
	WCHAR *temp = (void*)SDL_iconv_string("UTF-16LE", "UTF-8", s, utf8_size);
	WCHAR *buf = memdup(temp, utf16_size);
	SDL_free(temp);
	return buf;
}

static VFSNode *vfs_syspath_create_internal(char *path);

static void vfs_syspath_free(VFSNode *node) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	mem_free(pnode->path);
	mem_free(pnode->wpath);
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
	mem_free(errstr);
}

#define vfs_set_error_win32() _vfs_set_error_win32(_TAISEI_SRC_FILE, __LINE__)

static VFSInfo vfs_syspath_query(VFSNode *node) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	VFSInfo i = {0};

	if(!PathFileExists(pnode->wpath)) {
		i.exists = false;
		return i;
	}

	DWORD attrib = GetFileAttributes(pnode->wpath);

	if(attrib == INVALID_FILE_ATTRIBUTES) {
		vfs_set_error_win32();
		return VFSINFO_ERROR;
	}

	i.exists = true;
	i.is_dir = (bool)(attrib & FILE_ATTRIBUTE_DIRECTORY);

	return i;
}

static SDL_RWops *vfs_syspath_open(VFSNode *node, VFSOpenMode mode) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	mode &= VFS_MODE_RWMASK;
	SDL_RWops *rwops = SDL_RWFromFile(pnode->path, mode == VFS_MODE_WRITE ? "w" : "r");

	if(!rwops) {
		vfs_set_error_from_sdl();
	}

	return rwops;
}

static VFSNode *vfs_syspath_locate(VFSNode *node, const char *path) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	return vfs_syspath_create_internal(strjoin(pnode->path, "\\", path, NULL));
}

typedef struct VFSWin32IterData {
	HANDLE search_handle;
	char *last_result;
} VFSWin32IterData;

static const char* vfs_syspath_iter(VFSNode *node, void **opaque) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	VFSWin32IterData *idata;
	WIN32_FIND_DATA fdata;
	HANDLE search_handle;

	if(!*opaque) {
		char *pattern = strjoin(pnode->path, "\\*.*", NULL);
		wchar_t *wpattern = WIN_UTF8ToString(pattern);
		mem_free(pattern);
		search_handle = FindFirstFile(wpattern, &fdata);
		mem_free(wpattern);

		if(search_handle == INVALID_HANDLE_VALUE) {
			vfs_set_error_win32();
			return NULL;
		}

		idata = ALLOC(VFSWin32IterData, {
			.search_handle = search_handle,
			.last_result = WIN_StringToUTF8(fdata.cFileName),
		});
		*opaque = idata;

		if(!strcmp(idata->last_result, ".") || !strcmp(idata->last_result, "..")) {
			return vfs_syspath_iter(node, opaque);
		}

		return idata->last_result;
	}

	idata = *opaque;
	mem_free(idata->last_result);
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

		mem_free(idata->last_result);
		mem_free(idata);
	}
}

static char* vfs_syspath_repr(VFSNode *node) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	return strfmt("filesystem path (win32): %s", pnode->path);
}

static char* vfs_syspath_syspath(VFSNode *node) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);
	char *p = mem_strdup(pnode->path);
	vfs_syspath_normalize_inplace(p);
	return p;
}

static bool vfs_syspath_mkdir(VFSNode *node, const char *subdir) {
	auto pnode = VFS_NODE_CAST(VFSSysPathNode, node);

	if(!subdir) {
		subdir = "";
	}

	char *p = strjoin(pnode->path, "\\", subdir, NULL);
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

	mem_free(p);
	mem_free(wp);
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

static VFSNode *vfs_syspath_create_internal(char *path) {
	vfs_syspath_normalize_inplace(path);

	if(!vfs_syspath_validate(path)) {
		return NULL;
	}

	return &VFS_ALLOC(VFSSysPathNode, {
		.path = path,
		.wpath = WIN_UTF8ToString(path),
	})->as_generic;
}

VFSNode *vfs_syspath_create(const char *path) {
	return vfs_syspath_create_internal(mem_strdup(path));
}
