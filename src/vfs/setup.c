/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "build_config.h"
#include "public.h"
#include "setup.h"

static char* get_default_res_path(void) {
    char *res;

#ifdef RELATIVE
    res = SDL_GetBasePath();
    strappend(&res, DATA_PATH);
#else
    res = strdup(DATA_PATH);
#endif

    return res;
}

static void get_core_paths(char **res, char **storage) {
    if((*res = getenv("TAISEI_RES_PATH")) && **res) {
        *res = strdup(*res);
    } else {
        *res = get_default_res_path();
    }

    if((*storage = getenv("TAISEI_STORAGE_PATH")) && **storage) {
        *storage = strdup(*storage);
    } else {
        *storage = SDL_GetPrefPath("", "taisei");
    }
}

static struct pkg_loader_t {
    const char *const ext;
    bool (*mount)(const char *mp, const char *arg);
} pkg_loaders[] = {
    { ".zip",       vfs_mount_zipfile },
    { NULL },
};

static struct pkg_loader_t* find_loader(const char *str) {
    char buf[strlen(str) + 1];
    memset(buf, 0, sizeof(buf));

    for(char *p = buf; *str; ++p, ++str) {
        *p = tolower(*str);
    }

    for(struct pkg_loader_t *l = pkg_loaders; l->ext; ++l) {
        if(strendswith(buf, l->ext)){
            return l;
        }
    }

    return NULL;
}

static void load_packages(const char *dir, const char *unionmp) {
    // go over the packages in dir in alphapetical order and merge them into unionmp
    // e.g. files in 00-aaa.zip will be shadowed by files in 99-zzz.zip

    size_t numpaks = 0;
    char **paklist = vfs_dir_list_sorted(dir, &numpaks, vfs_dir_list_order_ascending, NULL);

    if(!paklist) {
        log_fatal("VFS error: %s", vfs_get_error());
    }

    for(size_t i = 0; i < numpaks; ++i) {
        const char *entry = paklist[i];
        struct pkg_loader_t *loader = find_loader(entry);

        if(loader == NULL) {
            continue;
        }

        log_info("Adding package: %s", entry);
        assert(loader->mount != NULL);

        char *tmp = strfmt("%s/%s", dir, entry);

        if(!loader->mount(unionmp, tmp)) {
            log_warn("VFS error: %s", vfs_get_error());
        }

        free(tmp);
    }

    vfs_dir_list_free(paklist, numpaks);
}

void vfs_setup(bool silent) {
    char *res_path, *storage_path;
    get_core_paths(&res_path, &storage_path);

    if(!silent) {
        log_info("Resource path: %s", res_path);
        log_info("Storage path: %s", storage_path);
    }

    char *p = NULL;

    struct mpoint_t {
        const char *dest;   const char *syspath;                            bool mkdir; bool loadpaks;
    } mpts[] = {
        // per-user directory, where configs, replays, screenshots, etc. get stored
        {"storage",         storage_path,                                   true,       false},

        // system-wide directory, contains all of the game assets
        {"resdirs",         res_path,                                       false,      true},

        // subpath of storage, files here override the global assets
        {"resdirs",         p = strfmt("%s/resources", storage_path),       true,       true},
        {NULL}
    };

    vfs_init();

    // temporary union of the "real" directories
    vfs_create_union_mountpoint("resdirs");

    // temporary union of the packages (e.g. zip files)
    vfs_create_union_mountpoint("respkgs");

    // permanent union of respkgs and resdirs
    // this way, files in any of the "real" directories always have priority over anything in packages
    vfs_create_union_mountpoint("res");

    for(struct mpoint_t *mp = mpts; mp->dest; ++mp) {
        if(mp->loadpaks) {
            // mount it to a temporary mountpoint to get a list of packages from this directory
            if(!vfs_mount_syspath("tmp", mp->syspath, mp->mkdir)) {
                log_fatal("Failed to mount '%s': %s", mp->syspath, vfs_get_error());
            }

            if(!vfs_query("tmp").is_dir) {
                log_warn("'%s' is not a directory", mp->syspath);
                vfs_unmount("tmp");
                continue;
            }

            // load all packages from this directory into the respkgs union
            load_packages("tmp", "respkgs");

            // now mount it to the intended destination, and remove the temporary mountpoint
            vfs_mount_alias(mp->dest, "tmp");
            vfs_unmount("tmp");
        } else {
            if(!vfs_mount_syspath(mp->dest, mp->syspath, mp->mkdir)) {
                log_fatal("Failed to mount '%s': %s", mp->syspath, vfs_get_error());
            }
        }
    }

    vfs_mkdir_required("storage/replays");
    vfs_mkdir_required("storage/screenshots");

    free(p);
    free(res_path);
    free(storage_path);

    // set up the final "res" union and get rid of the temporaries

    vfs_mount_alias("res", "respkgs");
    vfs_mount_alias("res", "resdirs");

    vfs_unmount("resdirs");
    vfs_unmount("respkgs");
}
