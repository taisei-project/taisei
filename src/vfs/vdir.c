
#include "vdir.h"

static void vfs_vdir_detach_node(VFSNode *vdir, VFSNode *node, bool unset) {
    assert(node->parent == vdir);

    if(unset) {
        hashtable_unset_string(vdir->vdir.contents, node->name);
    }

    free(node->name);
    node->name = NULL;
}

static void vfs_vdir_attach_node(VFSNode *vdir, const char *name, VFSNode *node) {
    assert(node->parent == NULL);
    assert(node->name == NULL);

    VFSNode *oldnode = hashtable_get_string(vdir->vdir.contents, name);

    if(oldnode) {
        vfs_vdir_detach_node(vdir, oldnode, true);
    }

    node->name = strdup(name);
    node->parent = vdir;

    hashtable_set_string(vdir->vdir.contents, name, node);
}

static VFSNode* vfs_vdir_locate(VFSNode *vdir, const char *path) {
    VFSNode *node;
    char mutpath[strlen(path)+1];
    char *primpath, *subpath;

    assert(vdir->type == VNODE_VDIR);

    strcpy(mutpath, path);
    vfs_path_split_left(mutpath, &primpath, &subpath);

    if(node = hashtable_get_string(vdir->vdir.contents, mutpath)) {
        return vfs_locate(node, subpath);
    }

    return NULL;
}

static const char* vfs_vdir_iter(VFSNode *vdir, void **opaque) {
    char *ret = NULL;

    if(!*opaque) {
        *opaque = hashtable_iter(vdir->vdir.contents);
    }

    if(!hashtable_iter_next((HashtableIterator*)*opaque, (void**)&ret, NULL)) {
        *opaque = NULL;
    }

    return ret;
}

static void vfs_vdir_iter_stop(VFSNode *vdir, void **opaque) {
    if(*opaque) {
        free(*opaque);
        *opaque = NULL;
    }
}

static VFSInfo vfs_vdir_query(VFSNode *vdir) {
    return (VFSInfo) {
        .exists = true,
        .is_dir = true,
    };
}

static void vfs_vdir_free(VFSNode *vdir) {
    Hashtable *ht = vdir->vdir.contents;
    HashtableIterator *i;
    VFSNode *child;

    for(i = hashtable_iter(ht); hashtable_iter_next(i, NULL, (void**)&child);) {
        vfs_vdir_detach_node(vdir, child, false);
        vfs_free(child);
    }

    hashtable_free(ht);
}

static bool vfs_vdir_mount(VFSNode *vdir, const char *mountpoint, VFSNode *subtree) {
    if(!mountpoint) {
        // merge attempt, unsupported
        return false;
    }

    vfs_vdir_attach_node(vdir, mountpoint, subtree);
    return true;
}

static char* vfs_vdir_repr(VFSNode *node) {
    return strdup("virtual directory");
}

static VFSNodeFuncs vfs_funcs_vdir = {
    .repr = vfs_vdir_repr,
    .query = vfs_vdir_query,
    .free = vfs_vdir_free,
    .mount = vfs_vdir_mount,
    .locate = vfs_vdir_locate,
    .iter = vfs_vdir_iter,
    .iter_stop = vfs_vdir_iter_stop,
};

void vfs_vdir_init(VFSNode *node) {
    node->type = VNODE_VDIR;
    node->funcs = &vfs_funcs_vdir;
    node->vdir.contents = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
}
