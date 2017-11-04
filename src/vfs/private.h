/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef TAISEI_VFS_PRIVATE
#define TAISEI_VFS_PRIVATE

/*
 *  This file should not be included by code outside of the vfs/ directory
 */

#ifndef DISABLE_ZIP
#include <zip.h>
#endif

#include "util.h"

#include "public.h"
#include "pathutil.h"

typedef struct VFSNode VFSNode;

typedef enum VFSNodeType {
    VNODE_VDIR,
    VNODE_SYSPATH,
    VNODE_UNION,
    VNODE_IPFS,
    VNODE_IPFSROOT,
#ifndef DISABLE_ZIP
    VNODE_ZIPFILE,
    VNODE_ZIPPATH,
#endif
} VFSNodeType;

typedef char* (*VFSReprFunc)(VFSNode*);
typedef void (*VFSFreeFunc)(VFSNode*);
typedef VFSInfo (*VFSQueryFunc)(VFSNode*);
typedef char* (*VFSSysPathFunc)(VFSNode*);

typedef bool (*VFSMountFunc)(VFSNode *mountroot, const char *subname, VFSNode *mountee);
typedef bool (*VFSUnmountFunc)(VFSNode *mountroot, const char *subname);

typedef VFSNode* (*VFSLocateFunc)(VFSNode *dirnode, const char* path);
typedef const char* (*VFSIterFunc)(VFSNode *dirnode, void **opaque);
typedef void (*VFSIterStopFunc)(VFSNode *dirnode, void **opaque);
typedef bool (*VFSMkDirFunc)(VFSNode *parent, const char *subdir);

typedef SDL_RWops* (*VFSOpenFunc)(VFSNode *filenode, VFSOpenMode mode);

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
    VFSNodeType type;
    VFSNodeFuncs *funcs;
    SDL_atomic_t refcount;

    union {
        void *data;

        // TODO: maybe somehow separate this

        struct {
            Hashtable *contents;
        } vdir;

        struct {
            char *path;
            wchar_t *wpath;
        } syspath;

        struct {
            struct {
                ListContainer *all;
                VFSNode *primary;
            } members;
        } vunion;
    };
} VFSNode;

typedef void (*VFSShutdownHandler)(void *arg);

extern VFSNode *vfs_root;

VFSNode* vfs_alloc(void);
void vfs_incref(VFSNode *node);
bool vfs_decref(VFSNode *node);
char* vfs_repr_node(VFSNode *node, bool try_syspath);

VFSNode* vfs_locate(VFSNode *root, const char *path);
VFSInfo vfs_query_node(VFSNode *node);
bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree);
const char* vfs_iter(VFSNode *node, void **opaque);
void vfs_iter_stop(VFSNode *node, void **opaque);

void vfs_set_error(char *fmt, ...) __attribute__((format(FORMAT_ATTR, 1, 2)));
void vfs_set_error_from_sdl(void);

void vfs_hook_on_shutdown(VFSShutdownHandler, void *arg);

void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix, const char *name);

#endif
