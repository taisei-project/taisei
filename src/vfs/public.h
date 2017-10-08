/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef TAISEI_VFS_PUBLIC
#define TAISEI_VFS_PUBLIC

#include <SDL.h>
#include <stdbool.h>

typedef struct VFSInfo {
    unsigned int error: 1;
    unsigned int exists : 1;
    unsigned int is_dir : 1;
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

bool vfs_create_union_mountpoint(const char *mountpoint);
bool vfs_mount_alias(const char *dst, const char *src);
bool vfs_mount_syspath(const char *mountpoint, const char *fspath, bool mkdir);
bool vfs_mount_zipfile(const char *mountpoint, const char *zippath);
bool vfs_unmount(const char *path);

VFSDir* vfs_dir_open(const char *path);
void vfs_dir_close(VFSDir *dir);
const char* vfs_dir_read(VFSDir *dir);

char** vfs_dir_list_sorted(const char *path, size_t *out_size, int (*compare)(const char**, const char**), bool (*filter)(const char*));
void vfs_dir_list_free(char **list, size_t size);
int vfs_dir_list_order_ascending(const char **a, const char **b);
int vfs_dir_list_order_descending(const char **a, const char **b);

char* vfs_repr(const char *path, bool try_syspath);
bool vfs_print_tree(SDL_RWops *dest, const char *path);

// these are defined in private.c, but need to be accessible from external code
void vfs_init(void);
void vfs_uninit(void);
const char* vfs_get_error(void);

#endif
