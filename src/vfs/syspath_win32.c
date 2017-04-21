
#ifndef UNICODE
    #define UNICODE
#endif

#ifndef _UNICODE
    #define _UNICODE
#endif

#include <windows.h>
#include <shlwapi.h>
#include <SDL.h>

#include "syspath.h"

// taken from SDL
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UTF-16LE", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", (char *)(S), SDL_strlen(S)+1)

static void vfs_syspath_init_internal(VFSNode *node, char *path, char *name);

static void vfs_syspath_free(VFSNode *node) {
    free(node->syspath.path);
    free(node->syspath.wpath);
}

static void _vfs_set_error_win32(const char *file, int line) {
    vfs_set_error("win32 error: %lu [%s:%i]", GetLastError(), file, line);
}

#define vfs_set_error_win32() _vfs_set_error_win32(__FILE__, __LINE__)

static VFSInfo vfs_syspath_query(VFSNode *node) {
    VFSInfo i = {0};

    if(!PathFileExists(node->syspath.wpath)) {
        i.exists = false;
        return i;
    }

    DWORD attrib = GetFileAttributes(node->syspath.wpath);

    if(attrib == INVALID_FILE_ATTRIBUTES) {
        vfs_set_error_win32();
        return VFSINFO_ERROR;
    }

    i.exists = true;
    i.is_dir = (bool)(attrib & FILE_ATTRIBUTE_DIRECTORY);

    return i;
}

static SDL_RWops* vfs_syspath_open(VFSNode *node, VFSOpenMode mode) {
    SDL_RWops *rwops = SDL_RWFromFile(node->syspath.path, mode == VFS_MODE_WRITE ? "w" : "r");

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
    vfs_syspath_init_internal(n, strfmt("%s%c%s", node->syspath.path, '\\', path), strdup(name));
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
        char *pattern = strjoin(node->syspath.path, "\\*.*", NULL);
        wchar_t *wpattern = WIN_UTF8ToString(pattern);
        log_debug("begin search: %s", pattern);
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
    return strfmt("filesystem path (win32): %s", node->syspath.path);
}

static char* vfs_syspath_syspath(VFSNode *node) {
    return strdup(node->syspath.path);
}

static bool vfs_syspath_mkdir(VFSNode *node, const char *subdir) {
    if(!subdir) {
        subdir = "";
    }

    char *p = strfmt("%s%c%s", node->syspath.path, '\\', subdir);
    wchar_t *wp = WIN_UTF8ToString(p);
    bool ok = CreateDirectory(wp, NULL);
    DWORD err = GetLastError();

    if(!ok && err == ERROR_ALREADY_EXISTS) {
        VFSNode *n = vfs_locate(node, subdir);

        if(n && vfs_query_node(n).is_dir) {
            ok = true;
        } else {
            vfs_set_error("%s already exists, and is not a directory", p);
        }

        if(node != n) {
            vfs_locate_cleanup(vfs_root, n);
        }
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

static void vfs_syspath_normalize(char *path) {
    // here we just remove redundant separators and convert forward slashes to back slashes.
    // no need to bother with . and ..

    // paths starting with two separators are a special case however (network shares)
    // and are handled appropriately.

    char buf[strlen(path)+1];
    char *bufptr = buf;
    char *pathptr = path;
    bool skip = false;

    memset(buf, 0, sizeof(buf));

    while(*pathptr) {
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

    strcpy(path, buf);
}

static void vfs_syspath_init_internal(VFSNode *node, char *path, char *name) {
    vfs_syspath_normalize(path);
    node->type = VNODE_SYSPATH;
    node->funcs = &vfs_funcs_syspath;
    node->syspath.path = path;
    node->syspath.wpath = WIN_UTF8ToString(path);
    node->name = name;
}

void vfs_syspath_init(VFSNode *node, const char *path) {
    vfs_syspath_init_internal(node, strdup(path), NULL);
}
