/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "ipfs_root.h"
#include "ipfs.h"

static char* vfs_ipfsroot_repr(VFSNode *node) {
    return strdup("IPFS root");
}

static char* vfs_ipfsroot_syspath(VFSNode *node) {
    return strdup("ipfs://");
}

static VFSInfo vfs_ipfsroot_query(VFSNode *node) {
    return (VFSInfo){ .is_dir = true };
}

static VFSNode* vfs_ipfsroot_locate(VFSNode *node, const char *path) {
    VFSNode *n = vfs_alloc(true);

    if(!vfs_ipfs_init(n, path)) {
        vfs_decref(n);
    }

    return n;
}

static VFSNodeFuncs vfs_funcs_ipfsroot = {
    .repr = vfs_ipfsroot_repr,
    .query = vfs_ipfsroot_query,
    .locate = vfs_ipfsroot_locate,
    .syspath = vfs_ipfsroot_syspath,
};

bool vfs_ipfsroot_init(VFSNode *node) {
    node->type = VNODE_IPFSROOT;
    node->funcs = &vfs_funcs_ipfsroot;
    return true;
}
