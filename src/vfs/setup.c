
#include "public.h"
#include "setup.h"

#define STATIC_RESOURCE_PREFIX PREFIX "/share/taisei/"

static char* get_default_res_path(void) {
    char *res;

#ifdef RELATIVE
    res = SDL_GetBasePath();
    strappend(&res, "data/");
#else
    res = strdup(STATIC_RESOURCE_PREFIX);
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

static bool filter_zip_ext(const char *str) {
    char buf[strlen(str) + 1];
    memset(buf, 0, sizeof(buf));

    for(char *p = buf; *str; ++p, ++str) {
        *p = tolower(*str);
    }

    return strendswith(buf, ".zip");
}

static void load_packages(const char *dir, const char *unionmp) {
    // go over the ZIPs in dir in alphapetical order and merge them into unionmp
    // e.g. files in 00-aaa.zip will be shadowed by files in 99-zzz.zip

    size_t numzips = 0;
    char **ziplist = vfs_dir_list_sorted(dir, &numzips, vfs_dir_list_order_ascending, filter_zip_ext);

    if(!ziplist) {
        log_fatal("VFS error: %s", vfs_get_error());
    }

    for(size_t i = 0; i < numzips; ++i) {
        const char *entry = ziplist[i];
        log_info("Adding package: %s", entry);

        char *tmp = strfmt("%s/%s", dir, entry);

        if(!vfs_mount_zipfile(unionmp, tmp)) {
            log_warn("VFS error: %s", vfs_get_error());
        }

        free(tmp);
    }

    vfs_dir_list_free(ziplist, numzips);
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
        const char *dest;   const char *syspath;                            bool mkdir; bool loadzips;
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

    // temporary union of the ZIP packages
    vfs_create_union_mountpoint("respkgs");

    // permanent union of respkgs and resdirs
    // this way, files in any of the "real" directories always have priority over anything in ZIPs
    vfs_create_union_mountpoint("res");

    for(struct mpoint_t *mp = mpts; mp->dest; ++mp) {
        if(mp->loadzips) {
            // mount it to a temporary mountpoint to get a list of ZIPs from this directory
            if(!vfs_mount_syspath("tmp", mp->syspath, mp->mkdir)) {
                log_fatal("Failed to mount '%s': %s", mp->syspath, vfs_get_error());
            }

            // load all ZIPs from this directory into the respkgs union
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
