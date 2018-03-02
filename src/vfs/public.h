/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include <stdbool.h>

#include "syspath_public.h"
#include "union_public.h"
#include "zipfile_public.h"

typedef struct VFSInfo {
	uint error: 1;
	uint exists : 1;
	uint is_dir : 1;
} VFSInfo;

#define VFSINFO_ERROR ((VFSInfo){.error = true, 0})

typedef enum VFSOpenMode {
	VFS_MODE_READ = 1,
	VFS_MODE_WRITE = 2,
	VFS_MODE_SEEKABLE  = 4,
} VFSOpenMode;

#define VFS_MODE_RWMASK (VFS_MODE_READ | VFS_MODE_WRITE)

typedef struct VFSDir VFSDir;

SDL_RWops* vfs_open(const char *path, VFSOpenMode mode);
VFSInfo vfs_query(const char *path);

bool vfs_mkdir(const char *path);
void vfs_mkdir_required(const char *path);

bool vfs_mount_alias(const char *dst, const char *src);
bool vfs_unmount(const char *path);

VFSDir* vfs_dir_open(const char *path) attr_nonnull(1) attr_nodiscard;
void vfs_dir_close(VFSDir *dir) attr_nonnull(1);
const char* vfs_dir_read(VFSDir *dir) attr_nonnull(1);

char** vfs_dir_list_sorted(const char *path, size_t *out_size, int (*compare)(const char**, const char**), bool (*filter)(const char*))
	attr_nonnull(1, 2, 3) attr_nodiscard;
void vfs_dir_list_free(char **list, size_t size);
int vfs_dir_list_order_ascending(const char **a, const char **b);
int vfs_dir_list_order_descending(const char **a, const char **b);

char* vfs_repr(const char *path, bool try_syspath) attr_nonnull(1) attr_nodiscard;
bool vfs_print_tree(SDL_RWops *dest, const char *path) attr_nonnull(1, 2);

// these are defined in private.c, but need to be accessible from external code
void vfs_init(void);
void vfs_shutdown(void);
const char* vfs_get_error(void) attr_returns_nonnull;
