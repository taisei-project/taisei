/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

/*
 *  This file should not be included by code outside of the vfs/ directory
 */

#include "util.h"

#include "public.h"
#include "pathutil.h"
#include "error.h"

typedef struct VFSNode VFSNode;
typedef struct VFSNodeFuncs VFSNodeFuncs;
typedef void (*VFSShutdownHandler)(void *arg);

struct VFSNodeFuncs {
	char*       (*repr)(VFSNode *node) attr_nonnull(1);
	void        (*free)(VFSNode *node) attr_nonnull(1);
	VFSInfo     (*query)(VFSNode *node) attr_nonnull(1);
	char*       (*syspath)(VFSNode *node) attr_nonnull(1);
	bool        (*mount)(VFSNode *mountroot, const char *subname, VFSNode *mountee) attr_nonnull(1, 3);
	bool        (*unmount)(VFSNode *mountroot, const char *subname) attr_nonnull(1);
	VFSNode*    (*locate)(VFSNode *dirnode, const char* path) attr_nonnull(1, 2);
	const char* (*iter)(VFSNode *dirnode, void **opaque) attr_nonnull(1);
	void        (*iter_stop)(VFSNode *dirnode, void **opaque) attr_nonnull(1);
	bool        (*mkdir)(VFSNode *parent, const char *subdir) attr_nonnull(1);
	SDL_RWops*  (*open)(VFSNode *filenode, VFSOpenMode mode) attr_nonnull(1);
};

struct VFSNode {
	VFSNodeFuncs *funcs;
	SDL_atomic_t refcount;
	void *data1;
	void *data2;
};

extern VFSNode *vfs_root;

VFSNode* vfs_alloc(void);
void vfs_incref(VFSNode *node) attr_nonnull(1);
bool vfs_decref(VFSNode *node);
bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
bool vfs_mount_or_decref(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
VFSNode* vfs_locate(VFSNode *root, const char *path) attr_nonnull(1, 2) attr_nodiscard;

// Light wrappers around the virtual functions, safe to call even on nodes that
// don't implement the corresponding method. "free" is not included, there should
// be no reason to call it. It wouldn't do what you'd expect anyway; use vfs_decref.
char* vfs_node_repr(VFSNode *node, bool try_syspath) attr_returns_allocated attr_nonnull(1);
VFSInfo vfs_node_query(VFSNode *node) attr_nonnull(1) attr_nodiscard attr_nonnull(1);
char* vfs_node_syspath(VFSNode *node) attr_nonnull(1) attr_returns_max_aligned attr_nodiscard attr_nonnull(1);
bool vfs_node_mount(VFSNode *mountroot, const char *subname, VFSNode *mountee) attr_nonnull(1, 3);
bool vfs_node_unmount(VFSNode *mountroot, const char *subname) attr_nonnull(1);
VFSNode* vfs_node_locate(VFSNode *root, const char *path) attr_nonnull(1, 2) attr_nodiscard;
const char* vfs_node_iter(VFSNode *node, void **opaque) attr_nonnull(1);
void vfs_node_iter_stop(VFSNode *node, void **opaque) attr_nonnull(1);
bool vfs_node_mkdir(VFSNode *parent, const char *subdir) attr_nonnull(1);
SDL_RWops* vfs_node_open(VFSNode *filenode, VFSOpenMode mode) attr_nonnull(1) attr_nodiscard;

void vfs_hook_on_shutdown(VFSShutdownHandler, void *arg);
void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix, const char *name) attr_nonnull(1, 2, 3, 4);
