
#include "private.h"
#include "vdir.h"

VFSNode *vfs_root;

typedef struct vfs_tls_s {
    char *error_str;
} vfs_tls_t;

static SDL_TLSID vfs_tls_id;
static vfs_tls_t *vfs_tls_fallback;

static void vfs_tls_free(void *vtls) {
    if(vtls) {
        vfs_tls_t *tls = vtls;
        free(tls->error_str);
        free(tls);
    }
}

static vfs_tls_t* vfs_tls_get(void) {
    if(vfs_tls_id) {
        return SDL_TLSGet(vfs_tls_id);
    }

    return vfs_tls_fallback;
}

void vfs_init(void) {
    vfs_root = vfs_alloc();
    vfs_vdir_init(vfs_root);
    vfs_root->name = strdup("");

    vfs_tls_id = SDL_TLSCreate();

    if(vfs_tls_id) {
        SDL_TLSSet(vfs_tls_id, calloc(1, sizeof(vfs_tls_t)), vfs_tls_free);
        vfs_tls_fallback = NULL;
    } else {
        log_warn("SDL_TLSCreate(): failed: %s", SDL_GetError());
        vfs_tls_fallback = calloc(1, sizeof(vfs_tls_t));
    }
}

void vfs_uninit(void) {
    vfs_free(vfs_root);
    vfs_tls_free(vfs_tls_fallback);

    vfs_root = NULL;
    vfs_tls_id = 0;
    vfs_tls_fallback = NULL;
}

VFSNode* vfs_alloc(void) {
    return calloc(1, sizeof(VFSNode));
}

void vfs_free(VFSNode *node) {
    if(!node) {
        return;
    }

    if(node->funcs && node->funcs->free) {
        node->funcs->free(node);
    }

    free(node->name);
    free(node);
}

VFSInfo vfs_query_node(VFSNode *node) {
    assert(node != NULL);
    assert(node->funcs != NULL);
    assert(node->funcs->query != NULL);

    return node->funcs->query(node);
}

VFSNode* vfs_locate(VFSNode *root, const char *path) {
    assert(root != NULL);
    assert(path != NULL);
    assert(root->funcs != NULL);

#ifndef NDEBUG
    char buf[strlen(path)+1];
    strcpy(buf, path);
    vfs_path_normalize(path, buf);
    assert(!strcmp(path, buf));
#endif

    if(!*path) {
        return root;
    }

    if(root->funcs->locate) {
        return root->funcs->locate(root, path);
    }

    return NULL;
}

void vfs_locate_cleanup(VFSNode *root, VFSNode *node) {
    if(node && !node->parent && node != root) {
        vfs_free(node);
    }
}

bool vfs_mount(VFSNode *root, const char *mountpoint, VFSNode *subtree) {
    VFSNode *mpnode;
    char buf[2][strlen(mountpoint)+1];
    char *mpbase, *mpname;
    bool result = false;

    mountpoint = vfs_path_normalize(mountpoint, buf[0]);
    strcpy(buf[1], buf[0]);
    vfs_path_split_right(buf[1], &mpbase, &mpname);

    if(mpnode = vfs_locate(root, mountpoint)) {
        // mountpoint already exists - try to merge with the target node

        if(mpnode->funcs->mount) {
            // expected to set error on failure
            result = mpnode->funcs->mount(mpnode, NULL, subtree);
        } else {
            result = false;
            vfs_set_error("Mountpoint '%s' already exists and does not support merging", mountpoint);
        }

        vfs_locate_cleanup(root, mpnode);
        return result;
    }

    if(mpnode = vfs_locate(root, mpbase)) {
        // try to become a subnode of parent (conventional mount)

        if(mpnode->funcs->mount) {
            // expected to set error on failure
            result = mpnode->funcs->mount(mpnode, mpname, subtree);
        } else {
            result = false;
            vfs_set_error("Parent directory '%s' of mountpoint '%s' does not support mounting", mpbase, mountpoint);
        }

        vfs_locate_cleanup(root, mpnode);
    }

    return result;
}

const char* vfs_iter(VFSNode *node, void **opaque) {
    if(node->funcs->iter) {
        return node->funcs->iter(node, opaque);
    }

    return NULL;
}

void vfs_iter_stop(VFSNode *node, void **opaque) {
    if(node->funcs->iter_stop) {
        node->funcs->iter_stop(node, opaque);
    }
}

VFSNode* vfs_find_root(VFSNode *node) {
    VFSNode *root = node, *r = node;

    while(r = r->parent)
        root = r;

    return root;
}

char* vfs_repr_node(VFSNode *node, bool try_syspath) {
    assert(node != NULL);
    assert(node->funcs != NULL);
    assert(node->funcs->repr != NULL);
    char *r;

    if(try_syspath && node->funcs->syspath) {
        if(r = node->funcs->syspath(node)) {
            return r;
        }
    }

    VFSInfo i = vfs_query_node(node);
    char *o = node->funcs->repr(node);

    r = strfmt("<%s (t:%i e:%i x:%i d:%i)>", o,
        node->type, i.error, i.exists, i.is_dir
    );

    free(o);
    return r;
}

void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix) {
    void *o = NULL;
    bool is_dir = vfs_query_node(root).is_dir;
    char *newprefix = strfmt("%s%s%s", prefix, root->name, is_dir ? (char[]){VFS_PATH_SEP, 0} : "");
    char *r;

    SDL_RWprintf(dest, "%s = %s\n", newprefix, r = vfs_repr_node(root, false));
    free(r);

    if(!is_dir) {
        free(newprefix);
        return;
    }

    for(const char *n; n = vfs_iter(root, &o);) {
        VFSNode *node = vfs_locate(root, n);
        if(node) {
            vfs_print_tree_recurse(dest, node, newprefix);
            vfs_locate_cleanup(root, node);
        }
    }

    vfs_iter_stop(root, &o);
    free(newprefix);
}

const char* vfs_get_error(void) {
    vfs_tls_t *tls = vfs_tls_get();
    return tls->error_str ? tls->error_str : "No error";
}

void vfs_set_error(char *fmt, ...) {
    vfs_tls_t *tls = vfs_tls_get();
    va_list args;
    va_start(args, fmt);
    char *err = vstrfmt(fmt, args);
    free(tls->error_str);
    tls->error_str = err;
    va_end(args);

    log_debug("%s", tls->error_str);
}

void vfs_set_error_from_sdl(void) {
    vfs_set_error("SDL error: %s", SDL_GetError());
}
