
#include "private.h"
#include "union.h"
#include "syspath.h"

typedef struct VFSDir {
    VFSNode *node;
    void *opaque;
} VFSDir;

bool vfs_create_union_mountpoint(const char *mountpoint) {
    VFSNode *unode = vfs_alloc();
    vfs_union_init(unode);

    if(!vfs_mount(vfs_root, mountpoint, unode)) {
        vfs_free(unode);
        return false;
    }

    return true;
}

bool vfs_mount_syspath(const char *mountpoint, const char *fspath, bool mkdir) {
    VFSNode *rdir = vfs_alloc();

    if(!vfs_syspath_init(rdir, fspath)) {
        vfs_set_error("Can't initialize path: %s", vfs_get_error());
        vfs_free(rdir);
        return false;
    }

    assert(rdir->funcs);
    assert(rdir->funcs->mkdir);

    if(mkdir && !rdir->funcs->mkdir(rdir, NULL)) {
        vfs_set_error("Can't create directory: %s", vfs_get_error());
        vfs_free(rdir);
        return false;
    }

    if(!vfs_mount(vfs_root, mountpoint, rdir)) {
        vfs_free(rdir);
        return false;
    }

    return true;
}

SDL_RWops* vfs_open(const char *path, VFSOpenMode mode) {
    SDL_RWops *rwops = NULL;
    char p[strlen(path)+1];
    path = vfs_path_normalize(path, p);
    VFSNode *node = vfs_locate(vfs_root, path);

    if(node) {
        assert(node->funcs != NULL);

        if(node->funcs->open) {
            // expected to set error on failure
            rwops = node->funcs->open(node, mode);
            vfs_locate_cleanup(vfs_root, node);
        } else {
            vfs_set_error("Node '%s' can't be opened as a file", path);
        }
    } else {
        vfs_set_error("Node '%s' does not exist", path);
    }

    return rwops;
}

VFSInfo vfs_query(const char *path) {
    char p[strlen(path)+1];
    path = vfs_path_normalize(path, p);
    VFSNode *node = vfs_locate(vfs_root, path);

    if(node) {
        // expected to set error on failure
        // not that e.g. a file not existing on a real filesystem
        // is not an error condition. If we can't tell whether it
        // exists or not, that is an error.

        VFSInfo i = vfs_query_node(node);
        vfs_locate_cleanup(vfs_root, node);
        return i;
    }

    vfs_set_error("Node '%s' does not exist", path);
    return VFSINFO_ERROR;
}

bool vfs_mkdir(const char *path) {
    char p[strlen(path)+1];
    path = vfs_path_normalize(path, p);
    VFSNode *node = vfs_locate(vfs_root, path);
    bool ok = false;

    if(node && node->funcs->mkdir) {
        ok = node->funcs->mkdir(node, NULL);
        vfs_locate_cleanup(vfs_root, node);
        return ok;
    }

    vfs_locate_cleanup(vfs_root, node);

    char *parent, *subdir;
    vfs_path_split_right(p, &parent, &subdir);
    node = vfs_locate(vfs_root, parent);

    if(node) {
        if(node->funcs->mkdir) {
            ok = node->funcs->mkdir(node, subdir);
            node->funcs->mkdir(node, subdir);
            return ok;
        } else {
            vfs_set_error("Node '%s' does not support creation of subdirectories", parent);
        }
    } else {
        vfs_set_error("Node '%s' does not exist", parent);
    }

    vfs_locate_cleanup(vfs_root, node);
    return false;
}

void vfs_mkdir_required(const char *path) {
    if(!vfs_mkdir(path)) {
        log_fatal("%s", vfs_get_error());
    }
}

char* vfs_repr(const char *path, bool try_syspath) {
    char buf[strlen(path)+1];
    path = vfs_path_normalize(path, buf);
    VFSNode *node = vfs_locate(vfs_root, path);

    if(node) {
        char *p = vfs_repr_node(node, try_syspath);
        vfs_locate_cleanup(vfs_root, node);
        return p;
    }

    vfs_set_error("Node '%s' does not exist", path);
    return NULL;
}

bool vfs_print_tree(SDL_RWops *dest, char *path) {
    char p[strlen(path)+3], *ignore;
    path = vfs_path_normalize(path, p);
    VFSNode *node = vfs_locate(vfs_root, path);

    if(!node) {
        vfs_set_error("Node '%s' does not exist", path);
        return false;
    }

    if(*path) {
        vfs_path_split_right(path, &path, &ignore);
        if(*path) {
            char *e = strchr(path, 0);
            *e++ = '/';
            *e = 0;
        }
        vfs_path_root_prefix(path);
    }

    vfs_print_tree_recurse(dest, node, path);
    return true;
}

VFSDir* vfs_dir_open(const char *path) {
    char p[strlen(path)+1];
    path = vfs_path_normalize(path, p);
    VFSNode *node = vfs_locate(vfs_root, path);

    if(node) {
        if(node->funcs->iter && vfs_query_node(node).is_dir) {
            VFSDir *d = calloc(1, sizeof(VFSDir));
            d->node = node;
            return d;
        } else {
            vfs_set_error("Node '%s' is not a directory", path);
        }

        vfs_locate_cleanup(vfs_root, node);
    } else {
        vfs_set_error("Node '%s' does not exist", path);
    }

    return NULL;
}

void vfs_dir_close(VFSDir *dir) {
    if(dir) {
        vfs_iter_stop(dir->node, &dir->opaque);
        vfs_locate_cleanup(vfs_root, dir->node);
    }
}

const char* vfs_dir_read(VFSDir *dir) {
    if(dir) {
        return vfs_iter(dir->node, &dir->opaque);
    }

    return NULL;
}
