/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

/*
 *  This file should not be included by code outside of the vfs/ directory
 */

#include "public.h"  // IWYU pragma: export
#include "pathutil.h"  // IWYU pragma: export
#include "error.h"  // IWYU pragma: export

#include "util.h"

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
	SDL_IOStream*  (*open)(VFSNode *filenode, VFSOpenMode mode) attr_nonnull(1);
};

struct VFSNode {
	char as_generic[0];
	const VFSNodeFuncs *funcs;
	SDL_AtomicInt refcount;
};

#define VFS_NODE_TYPE_FUNCS_NAME(_typename) _vfs_funcs_##_typename

#define VFS_NODE_TYPE(_typename, ...) \
	typedef struct _typename { \
		VFSNode as_generic; \
		struct __VA_ARGS__; \
	} _typename; \
	extern const VFSNodeFuncs VFS_NODE_TYPE_FUNCS_NAME(_typename)

#define VFS_NODE_FUNCS(_typename, ...) \
	const VFSNodeFuncs VFS_NODE_TYPE_FUNCS_NAME(_typename) = __VA_ARGS__

#define IS_OF_VFS_NODE_BASE_TYPE(_node) \
	__builtin_types_compatible_p(VFSNode, typeof(*(_node)))

#define IS_OF_VFS_NODE_DERIVED_TYPE(_node) \
	(__builtin_types_compatible_p(VFSNode, typeof((_node)->as_generic)) \
		&& offsetof(typeof(*(_node)), as_generic) == 0)

#define IS_OF_VFS_NODE_TYPE(_node) \
	IS_OF_VFS_NODE_BASE_TYPE(_node) || IS_OF_VFS_NODE_DERIVED_TYPE(_node)

#define VFS_NODE_ASSERT_VALID(_node) \
	static_assert(IS_OF_VFS_NODE_TYPE(_node), "Expected a VFSNode or derived type")

#define VFS_NODE_AS_BASE(_node) ({ \
	VFS_NODE_ASSERT_VALID(_node); \
	__builtin_choose_expr(IS_OF_VFS_NODE_BASE_TYPE(_node), (_node), &(_node)->as_generic); \
})

#define VFS_NODE_TRY_CAST(_typename, _node) ({ \
	VFSNode *_base_node = _node; \
	static_assert(IS_OF_VFS_NODE_DERIVED_TYPE( (&(_typename){}) ), \
		"Typename must be of a VFSNode-derived type"); \
	_typename* _n = UNION_CAST(VFSNode*, _typename*, NOT_NULL(_base_node)); \
	LIKELY(_n->as_generic.funcs == &VFS_NODE_TYPE_FUNCS_NAME(_typename))? _n : NULL; \
})

#define VFS_NODE_CAST(_typename, _node) \
	NOT_NULL(VFS_NODE_TRY_CAST(_typename, _node))

#define VFS_ALLOC(_typename, ...) ({ \
	auto _node = ALLOC(_typename, ##__VA_ARGS__); \
	static_assert(IS_OF_VFS_NODE_DERIVED_TYPE(_node), \
		"Typename must be of a VFSNode-derived type"); \
	_node->as_generic.funcs = &VFS_NODE_TYPE_FUNCS_NAME(_typename); \
	vfs_incref(_node); \
	_node; \
})


extern VFSNode *vfs_root;

void vfs_incref(VFSNode *node) attr_nonnull(1);
bool vfs_decref(VFSNode *node);
bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
bool vfs_mount_or_decref(VFSNode *root, const char *mountpoint, VFSNode *subtree) attr_nonnull(1, 3) attr_nodiscard;
VFSNode *vfs_locate(VFSNode *root, const char *path) attr_nonnull(1, 2) attr_nodiscard;

// Light wrappers around the virtual functions, safe to call even on nodes that
// don't implement the corresponding method. "free" is not included, there should
// be no reason to call it. It wouldn't do what you'd expect anyway; use vfs_decref.
char *vfs_node_repr(VFSNode *node, bool try_syspath) attr_returns_allocated attr_nonnull(1);
VFSInfo vfs_node_query(VFSNode *node) attr_nonnull(1) attr_nodiscard attr_nonnull(1);
char *vfs_node_syspath(VFSNode *node) attr_nonnull(1) attr_returns_max_aligned attr_nodiscard attr_nonnull(1);
bool vfs_node_mount(VFSNode *mountroot, const char *subname, VFSNode *mountee) attr_nonnull(1, 3);
bool vfs_node_unmount(VFSNode *mountroot, const char *subname) attr_nonnull(1);
VFSNode *vfs_node_locate(VFSNode *root, const char *path) attr_nonnull(1, 2) attr_nodiscard;
const char *vfs_node_iter(VFSNode *node, void **opaque) attr_nonnull(1);
void vfs_node_iter_stop(VFSNode *node, void **opaque) attr_nonnull(1);
bool vfs_node_mkdir(VFSNode *parent, const char *subdir) attr_nonnull(1);
SDL_IOStream *vfs_node_open(VFSNode *filenode, VFSOpenMode mode) attr_nonnull(1) attr_nodiscard;

// NOTE: convenience wrappers added on demand

#define vfs_incref(node) \
	vfs_incref(VFS_NODE_AS_BASE(node))

#define vfs_decref(node) \
	vfs_decref(VFS_NODE_AS_BASE(node))

#define vfs_locate(root, path) \
	vfs_locate(VFS_NODE_AS_BASE(root), path)

#define vfs_node_repr(node, try_syspath) \
	vfs_node_repr(VFS_NODE_AS_BASE(node), try_syspath)


void vfs_hook_on_shutdown(VFSShutdownHandler, void *arg);
void vfs_print_tree_recurse(SDL_IOStream *dest, VFSNode *root, char *prefix, const char *name) attr_nonnull(1, 2, 3, 4);
