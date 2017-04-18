
#ifndef TAISEI_VFS_PRIVATE
#define TAISEI_VFS_PRIVATE

/*
 *  This file should not be included by code outside of the vfs/ directory
 */

#include "util.h"

#include "public.h"
#include "pathutil.h"

typedef struct VFSNode VFSNode;

typedef enum VFSNodeType {
    VNODE_VDIR,
    VNODE_SYSPATH,
    VNODE_UNION,
} VFSNodeType;

typedef char* (*VFSReprFunc)(VFSNode*);
typedef void (*VFSFreeFunc)(VFSNode*);
typedef VFSInfo (*VFSQueryFunc)(VFSNode*);
typedef bool (*VFSMountFunc)(VFSNode *mountroot, const char *subname, VFSNode *mountee);
typedef char* (*VFSSysPathFunc)(VFSNode*);

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
    VFSSysPathFunc syspath;
    VFSLocateFunc locate;
    VFSIterFunc iter;
    VFSIterStopFunc iter_stop;
    VFSMkDirFunc mkdir;
    VFSOpenFunc open;
} VFSNodeFuncs;

typedef struct VFSNode {
    VFSNodeType type;
    VFSNode *parent;
    VFSNodeFuncs *funcs;
    char *name;

    union {
        // TODO: maybe somehow separate this

        struct {
            Hashtable *contents;
        } vdir;

        struct {
            char *path;
        } syspath;

        struct {
            struct {
                ListContainer *all;
                VFSNode *primary;
            } members;
        } vunion;
    };
} VFSNode;

extern VFSNode *vfs_root;

VFSNode* vfs_alloc(void);
void vfs_free(VFSNode *node);
char* vfs_repr(VFSNode *node);
VFSNode* vfs_locate(VFSNode *root, const char *path);
void vfs_locate_cleanup(VFSNode *root, VFSNode *node);
VFSInfo vfs_query_node(VFSNode *node);
bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree);
const char* vfs_iter(VFSNode *node, void **opaque);
void vfs_iter_stop(VFSNode *node, void **opaque);
VFSNode* vfs_find_root(VFSNode *node);

void vfs_set_error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
void vfs_set_error_from_sdl(void);

void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix);

#endif
