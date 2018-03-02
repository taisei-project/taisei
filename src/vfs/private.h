/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

/*
 *  This file should not be included by code outside of the vfs/ directory
 */

#include "util.h"

#include "public.h"
#include "pathutil.h"

typedef struct VFSNode VFSNode;

typedef char* (*VFSReprFunc)(VFSNode*) attr_nonnull(1);
typedef void (*VFSFreeFunc)(VFSNode*) attr_nonnull(1);
typedef VFSInfo (*VFSQueryFunc)(VFSNode*) attr_nonnull(1);
typedef char* (*VFSSysPathFunc)(VFSNode*) attr_nonnull(1);
typedef bool (*VFSMountFunc)(VFSNode *mountroot, const char *subname, VFSNode *mountee) attr_nonnull(1, 3);
typedef bool (*VFSUnmountFunc)(VFSNode *mountroot, const char *subname) attr_nonnull(1);
typedef VFSNode* (*VFSLocateFunc)(VFSNode *dirnode, const char* path) attr_nonnull(1, 2);
typedef const char* (*VFSIterFunc)(VFSNode *dirnode, void **opaque) attr_nonnull(1);
typedef void (*VFSIterStopFunc)(VFSNode *dirnode, void **opaque) attr_nonnull(1);
typedef bool (*VFSMkDirFunc)(VFSNode *parent, const char *subdir) attr_nonnull(1);
typedef SDL_RWops* (*VFSOpenFunc)(VFSNode *filenode, VFSOpenMode mode) attr_nonnull(1);

typedef struct VFSNodeFuncs {
	VFSReprFunc repr;
	VFSFreeFunc free;
	VFSQueryFunc query;
	VFSMountFunc mount;
	VFSUnmountFunc unmount;
	VFSSysPathFunc syspath;
	VFSLocateFunc locate;
	VFSIterFunc iter;
	VFSIterStopFunc iter_stop;
	VFSMkDirFunc mkdir;
	VFSOpenFunc open;
} VFSNodeFuncs;

typedef struct VFSNode {
	VFSNodeFuncs *funcs;
	SDL_atomic_t refcount;
	void *data1;
	void *data2;
} VFSNode;

typedef void (*VFSShutdownHandler)(void *arg);

extern VFSNode *vfs_root;

VFSNode* vfs_alloc(void);
void vfs_incref(VFSNode *node) attr_nonnull(1);
bool vfs_decref(VFSNode *node);
char* vfs_repr_node(VFSNode *node, bool try_syspath) attr_nonnull(1);
VFSNode* vfs_locate(VFSNode *root, const char *path) attr_nodiscard;
VFSInfo vfs_query_node(VFSNode *node) attr_nonnull(1) attr_nodiscard;
bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
bool vfs_mount_or_decref(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
const char* vfs_iter(VFSNode *node, void **opaque) attr_nonnull(1);
void vfs_iter_stop(VFSNode *node, void **opaque) attr_nonnull(1);

void vfs_set_error(char *fmt, ...) attr_printf(1, 2);
void vfs_set_error_from_sdl(void);

void vfs_hook_on_shutdown(VFSShutdownHandler, void *arg);

void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix, const char *name) attr_nonnull(1, 2, 3, 4);
