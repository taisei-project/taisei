/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "private.h"
#include "vdir.h"

VFSNode *vfs_root;

typedef struct vfs_tls_s {
    char *error_str;
} vfs_tls_t;

typedef struct vfs_shutdownhook_t {
    List refs;
    VFSShutdownHandler func;
    void *arg;
} vfs_shutdownhook_t;

static SDL_TLSID vfs_tls_id;
static vfs_tls_t *vfs_tls_fallback;
static vfs_shutdownhook_t *shutdown_hooks;

static void vfs_free(VFSNode *node);

static void vfs_tls_free(void *vtls) {
    if(vtls) {
        vfs_tls_t *tls = vtls;
        free(tls->error_str);
        free(tls);
    }
}

static vfs_tls_t* vfs_tls_get(void) {
    if(vfs_tls_id) {
        vfs_tls_t *tls = SDL_TLSGet(vfs_tls_id);

        if(!tls) {
            SDL_TLSSet(vfs_tls_id, calloc(1, sizeof(vfs_tls_t)), vfs_tls_free);
            tls = SDL_TLSGet(vfs_tls_id);
        }

        assert(tls != NULL);
        return tls;
    }

    assert(vfs_tls_fallback != NULL);
    return vfs_tls_fallback;
}

void vfs_init(void) {
    vfs_root = vfs_alloc();
    vfs_vdir_init(vfs_root);

    vfs_tls_id = SDL_TLSCreate();

    if(vfs_tls_id) {
        vfs_tls_fallback = NULL;
    } else {
        log_warn("SDL_TLSCreate(): failed: %s", SDL_GetError());
        vfs_tls_fallback = calloc(1, sizeof(vfs_tls_t));
    }
}

static void call_shutdown_hook(void **vlist, void *vhook) {
    vfs_shutdownhook_t *hook = vhook;
    hook->func(hook->arg);
    delete_element(vlist, vhook);
}

void vfs_shutdown(void) {
    delete_all_elements((void**)&shutdown_hooks, call_shutdown_hook);

    vfs_decref(vfs_root);
    vfs_tls_free(vfs_tls_fallback);

    vfs_root = NULL;
    vfs_tls_id = 0;
    vfs_tls_fallback = NULL;
}

void vfs_hook_on_shutdown(VFSShutdownHandler func, void *arg) {
    vfs_shutdownhook_t *hook = create_element_at_end((void**)&shutdown_hooks, sizeof(vfs_shutdownhook_t));
    hook->func = func;
    hook->arg = arg;
}

VFSNode* vfs_alloc(void) {
    VFSNode *node = calloc(1, sizeof(VFSNode));
    vfs_incref(node);
    return node;
}

static void vfs_free(VFSNode *node) {
    if(!node) {
        return;
    }

    if(node->funcs && node->funcs->free) {
        node->funcs->free(node);
    }

    free(node);
}

void vfs_incref(VFSNode *node) {
    assert(node != NULL);
    SDL_AtomicIncRef(&node->refcount);
}

bool vfs_decref(VFSNode *node) {
    if(!node) {
        return true;
    }

    if(SDL_AtomicDecRef(&node->refcount)) {
        vfs_free(node);
        return true;
    }

    return false;
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
        vfs_incref(root);
        return root;
    }

    if(root->funcs->locate) {
        return root->funcs->locate(root, path);
    }

    return NULL;
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

        vfs_decref(mpnode);
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

        vfs_decref(mpnode);
    }

    return result;
}

bool vfs_mount_or_decref(VFSNode *root, const char *mountpoint, VFSNode *node) {
    if(!vfs_mount(vfs_root, mountpoint, node)) {
        vfs_decref(node);
        return false;
    }

    return true;
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

    r = strfmt("<%s (e:%i x:%i d:%i)>", o,
        i.error, i.exists, i.is_dir
    );

    free(o);
    return r;
}

void vfs_print_tree_recurse(SDL_RWops *dest, VFSNode *root, char *prefix, const char *name) {
    void *o = NULL;
    bool is_dir = vfs_query_node(root).is_dir;
    char *newprefix = strfmt("%s%s%s", prefix, name, is_dir ? (char[]){VFS_PATH_SEP, 0} : "");
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
            vfs_print_tree_recurse(dest, node, newprefix, n);
            vfs_decref(node);
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
