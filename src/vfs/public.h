/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "syspath_public.h"  // IWYU pragma: export
#include "union_public.h"  // IWYU pragma: export
#include "zipfile_public.h"  // IWYU pragma: export
#include "readonly_wrapper_public.h"  // IWYU pragma: export

#include "util/callchain.h"

#include <SDL3/SDL.h>

typedef struct VFSInfo {
	uchar error       : 1;
	uchar exists      : 1;
	uchar is_dir      : 1;
	uchar is_readonly : 1;
} VFSInfo;

#define VFSINFO_ERROR ((VFSInfo) { .error = true, 0 })

typedef enum VFSOpenMode {
	VFS_MODE_READ = 1,
	VFS_MODE_WRITE = 2,
	VFS_MODE_SEEKABLE  = 4,
} VFSOpenMode;

typedef enum VFSSyncMode {
	VFS_SYNC_LOAD  = 1,
	VFS_SYNC_STORE = 0,
} VFSSyncMode;

#define VFS_MODE_RWMASK (VFS_MODE_READ | VFS_MODE_WRITE)

typedef struct VFSDir VFSDir;

SDL_IOStream * vfs_open(const char *path, VFSOpenMode mode);
VFSInfo vfs_query(const char *path);

bool vfs_mkdir(const char *path);
void vfs_mkdir_required(const char *path);
bool vfs_mkparents(const char *path);

bool vfs_mount_alias(const char *dst, const char *src);
bool vfs_unmount(const char *path);

VFSDir* vfs_dir_open(const char *path) attr_nonnull(1) attr_nodiscard;
void vfs_dir_close(VFSDir *dir);
const char* vfs_dir_read(VFSDir *dir) attr_nonnull(1);

void* vfs_dir_walk(const char *path, void* (*visit)(const char *path, void *arg), void *arg);

char** vfs_dir_list_sorted(const char *path, size_t *out_size, int (*compare)(const void*, const void*), bool (*filter)(const char*))
	attr_nonnull(1, 2, 3) attr_nodiscard;
void vfs_dir_list_free(char **list, size_t size);
int vfs_dir_list_order_ascending(const void *a, const void *b);
int vfs_dir_list_order_descending(const void *a, const void *b);

char* vfs_repr(const char *path, bool try_syspath) attr_nonnull(1) attr_nodiscard;
bool vfs_print_tree(SDL_IOStream *dest, const char *path) attr_nonnull(1, 2);

// these are defined in private.c, but need to be accessible from external code
void vfs_init(void);
void vfs_shutdown(void);
bool vfs_initialized(void);
const char* vfs_get_error(void) attr_returns_nonnull;

void vfs_sync(VFSSyncMode mode, CallChain next);
