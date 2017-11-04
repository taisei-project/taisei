/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

/*
 *  NOTE: This code relies on the "ipfs" command-line utility from the IPFS
 *        reference implementation. One could also write a curl-based backend,
 *        so that HTTP-based public gateways can be used in case IPFS is not
 *        installed locally.
 */

#include <ctype.h>

#include "ipfs.h"
#include "rwops/all.h"

#ifndef IPFS_CMD
#define IPFS_CMD "ipfs"
#endif

typedef struct IPFSNodeData {
    Hashtable *entries;
    VFSInfo info;
    char *path;
    // char hash[IPFS_HASH_SIZE];
} IPFSNodeData;

static struct {
    Hashtable *pathmap;
    SDL_mutex *pathmap_mtx;
    SDL_mutex *cmd_mtx;
} global;

static bool vfs_ipfs_init_internal(VFSNode *node, char *path);

static SDL_RWops* vfs_ipfs_cmd(const char *cmd, const char *path) {
    SDL_RWops *rw = NULL;
    char *fullcmd = strfmt(IPFS_CMD " %s %s", cmd, path);

    log_debug("COMMAND: { %s }", fullcmd);

    // XXX: it's very unstable without this mutex (libc segfaults in random threads),
    //      but with it the async loading is completely worthless.
    SDL_LockMutex(global.cmd_mtx);
    rw = SDL_RWpopen(fullcmd, "r");
    SDL_UnlockMutex(global.cmd_mtx);

    if(!rw) {
        vfs_set_error("IPFS command %s %s failed: %s", cmd, path, SDL_GetError());
    }

    free(fullcmd);
    return rw;
}

static bool vfs_ipfs_cmd_parselines(const char *cmd, const char *path, bool (*callback)(const char*, void*), void *arg) {
    bool ok = true;
    SDL_RWops *rw = vfs_ipfs_cmd(cmd, path);

    if(!rw) {
        return false;
    }

    char buffer[256];

    while(SDL_RWgets(rw, buffer, sizeof(buffer))) {
        char *ptr = buffer;
        // log_debug("BUFFER: { %s }", buffer);

        while(strchr("\r\n", *(ptr = strchr(ptr, 0) - 1))) {
            *ptr = 0;
        }

        if(!*buffer) {
            continue;
        }

        if(!callback(buffer, arg)) {
            ok = false;
            break;
        }
    }

    if(SDL_RWclose(rw)) {
        vfs_set_error("IPFS subprocess exited abnormally");
        return false;
    }

    return ok;
}

static bool vfs_ipfs_subpath_ok(const char *subpath, bool null_ok) {
    if(*subpath) {
        if(*subpath != '/') {
            vfs_set_error("IPFS subpath doesn't start with /: %s", subpath);
            return false;
        }

        for(const char *c = subpath + 1; *c; ++c) {
            // XXX: the actual IPFS restrictions aren't as strict of course
            // but we need to be very cautious here: this will be passed to the shell!
            if(!isalnum(*c) && !strchr("/._-", *c)) {
                vfs_set_error("Bad character %c in IPFS path: %s", *c, subpath);
                return false;
            }
        }
    } else if(!null_ok) {
        vfs_set_error("IPFS subpath is null");
        return false;
    }

    return true;
}

static bool vfs_ipfs_path_ok(const char *path) {
    // XXX: ipns is not supported

    char hashbuf[IPFS_HASH_SIZE];
    const char *subpath;

    strlcpy(hashbuf, path, sizeof(hashbuf));

    if(strlen(hashbuf) != sizeof(hashbuf) - 1) {
        vfs_set_error("IPFS path is too short: %s", path);
        return false;
    }

    for(char *c = hashbuf; *c; ++c) {
        if(!isalnum(*c)) {
            vfs_set_error("Bad character '%c' in IPFS path (hash): %s", *c, path);
            return false;
        }
    }

    subpath = path + sizeof(hashbuf) - 1;
    return vfs_ipfs_subpath_ok(subpath, true);
}

static void vfs_ipfs_free(VFSNode *node) {
    IPFSNodeData *nd = node->data;
    free(nd->path);

    if(nd->entries) {
        // hashtable_foreach(nd->entries, hashtable_iter_free_data, NULL);
        hashtable_free(nd->entries);
    }

    free(nd);
}

static VFSInfo vfs_ipfs_query(VFSNode *node) {
    IPFSNodeData *nd = node->data;
    return nd->info;
}

static char* vfs_ipfs_repr(VFSNode *node) {
    IPFSNodeData *nd = node->data;
    return strfmt("IPFS path: %s", nd->path);
}

static char* vfs_ipfs_syspath(VFSNode *node) {
    IPFSNodeData *nd = node->data;
    return strjoin("ipfs://", nd->path, NULL);
}

static bool vfs_ipfs_register_node(VFSNode *node) {
    IPFSNodeData *nd = node->data;

    SDL_LockMutex(global.pathmap_mtx);
    VFSNode *node2 = hashtable_get_string(global.pathmap, nd->path);

    if(node2 != NULL) {
        if(node2 != node) {
            SDL_UnlockMutex(global.pathmap_mtx);
            log_warn("Another node with IPFS path %s is already registered!", nd->path);
            return false;
        } else {
            SDL_UnlockMutex(global.pathmap_mtx);
            return true;
        }
    }

    vfs_incref(node);
    hashtable_set_string(global.pathmap, nd->path, node);
    SDL_UnlockMutex(global.pathmap_mtx);

    log_debug("Registered node %s", nd->path);
    return true;
}

static VFSNode* vfs_ipfs_get_cached(const char *path) {
    SDL_LockMutex(global.pathmap_mtx);
    VFSNode *n = hashtable_get_string(global.pathmap, path);
    SDL_UnlockMutex(global.pathmap_mtx);
    log_debug("Cache %s: %s", n ? "hit" : "miss", path);

    if(n) {
        vfs_incref(n);
    }

    return n;
}

static VFSNode* vfs_ipfs_locate(VFSNode *node, const char *path) {
    IPFSNodeData *nd = node->data;

    if(!nd->entries) {
        vfs_set_error("IPFS path %s is not a directory (or it's an empty one)", nd->path);
        return NULL;
    }

    VFSNode *n;
    char buf[strlen(path)+1], fullpath[strlen(path) + strlen(nd->path) + 2];
    char *primpath, *subpath;
    strcpy(buf, path);
    strip_trailing_slashes(buf);

    strcpy(fullpath, nd->path);
    strcat(fullpath, "/");
    strcat(fullpath, buf);
    n = vfs_ipfs_get_cached(fullpath);

    if(n != NULL) {
        return n;
    }

    vfs_path_split_left(buf, &primpath, &subpath);

    if(!hashtable_get_string(nd->entries, buf)) {
        vfs_set_error("IPFS directory %s doesn't contain %s", nd->path, buf);
        return NULL;
    }

    strcpy(fullpath, nd->path);
    strcat(fullpath, "/");
    strcat(fullpath, buf);
    n = vfs_ipfs_get_cached(fullpath);

    if(n == NULL) {
        n = vfs_alloc();

        if(!vfs_ipfs_init_internal(n, strdup(fullpath))) {
            vfs_decref(n);
            n = NULL;
        }
    }

    if(n != NULL && *subpath) {
        VFSNode *o = n;
        n = o->funcs->locate(n, subpath);
        vfs_decref(o);
    }

    return n;
}

static const char* vfs_ipfs_iter(VFSNode *node, void **opaque) {
    IPFSNodeData *nd = node->data;
    char *ret = NULL;

    if(!*opaque) {
        if(!nd->entries) {
            return NULL;
        }

        *opaque = hashtable_iter(nd->entries);
    }

    if(!hashtable_iter_next((HashtableIterator*)*opaque, (void**)&ret, NULL)) {
        *opaque = NULL;
    }

    if(ret) {
        strip_trailing_slashes(ret);
    }

    return ret;
}

static void vfs_ipfs_iter_stop(VFSNode *node, void **opaque) {
    if(*opaque) {
        free(*opaque);
        *opaque = NULL;
    }
}

static SDL_RWops* vfs_ipfs_open(VFSNode *node, VFSOpenMode mode) {
    IPFSNodeData *nd = node->data;

    if(mode & VFS_MODE_WRITE) {
        vfs_set_error("IPFS is read-only");
        return NULL;
    }

    if(nd->entries) {
        vfs_set_error("IPFS path %s is a directory", nd->path);
        return NULL;
    }

    SDL_RWops *rw = vfs_ipfs_cmd("cat", nd->path);

    if(!rw) {
        return NULL;
    }

    if(!(mode & VFS_MODE_SEEKABLE)) {
        return rw;
    }

    SDL_RWops *bufrw = SDL_RWCopyToBuffer(rw);
    SDL_RWclose(rw);
    return bufrw;
}

struct filldir_arg {
    Hashtable *ht;
    bool error;
};

static bool vfs_ipfs_fill_dir(const char *line, void *varg) {
    struct filldir_arg *arg = varg;
    Hashtable *ht = arg->ht;

    char buffer[strlen(line) + 1];
    char *bufptr = buffer;
    strcpy(buffer, line);

    char *hash = strtok_r(NULL, " ", &bufptr);
    strtok_r(NULL, " ", &bufptr); // ignore size
    char *path = strtok_r(NULL, " ", &bufptr);

    if(path == NULL) {
        return true;
    }

    if(!vfs_ipfs_path_ok(hash)) {
        vfs_set_error("Line begins with an invalid hash: %s", line);
        arg->error = true;
        return false;
    }

    vfs_path_normalize_inplace(path);
    strip_trailing_slashes(path);

    if(!ht) {
        arg->ht = ht = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
    }

    hashtable_set_string(ht, path, path /* not used for now; anything non-null will do */);

    return true;
}

static VFSNodeFuncs vfs_funcs_ipfs = {
    .repr = vfs_ipfs_repr,
    .query = vfs_ipfs_query,
    .free = vfs_ipfs_free,
    .locate = vfs_ipfs_locate,
    .syspath = vfs_ipfs_syspath,
    .iter = vfs_ipfs_iter,
    .iter_stop = vfs_ipfs_iter_stop,
    .open = vfs_ipfs_open,
};

static bool vfs_ipfs_init_internal(VFSNode *node, char *path) {
    vfs_path_normalize_inplace(path);

    struct filldir_arg state = { 0 };
    vfs_ipfs_cmd_parselines("ls", path, vfs_ipfs_fill_dir, &state);

    if(state.error) {
        free(path);
        hashtable_free(state.ht);
        return false;
    }

    IPFSNodeData *nd = malloc(sizeof(IPFSNodeData));
    nd->entries = state.ht;
    nd->path = path;
    nd->info = (VFSInfo) {
        .exists = true,
        .is_dir = (nd->entries != NULL),
        .error = false,
    };

    node->type = VNODE_IPFS;
    node->funcs = &vfs_funcs_ipfs;
    node->data = nd;

    vfs_ipfs_register_node(node);
    return true;
}

bool vfs_ipfs_init(VFSNode *node, const char *path) {
    if(!vfs_ipfs_path_ok(path)) {
        return false;
    }

    char *npath = strdup(path);
    vfs_path_normalize_inplace(npath);
    return vfs_ipfs_init_internal(node, npath);
}

void* vfs_ipfs_cleanup_callback(void *key, void *data, void *arg) {
    VFSNode *node = data;
    vfs_decref(node);
    return NULL;
}

void vfs_ipfs_global_cleanup(void *arg) {
    if(global.pathmap) {
        hashtable_foreach(global.pathmap, vfs_ipfs_cleanup_callback, NULL);
        hashtable_free(global.pathmap);
    }

    SDL_DestroyMutex(global.pathmap_mtx);
    SDL_DestroyMutex(global.cmd_mtx);
}

void vfs_ipfs_global_init(void) {
    if(global.pathmap) {
        return;
    }

    if(!(global.pathmap_mtx = SDL_CreateMutex())) {
        log_warn("SDL_CreateMutex() failed: %s", SDL_GetError());
    }

    if(!(global.cmd_mtx = SDL_CreateMutex())) {
        log_warn("SDL_CreateMutex() failed: %s", SDL_GetError());
    }

    global.pathmap = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
    vfs_hook_on_shutdown(vfs_ipfs_global_cleanup, NULL);
}
