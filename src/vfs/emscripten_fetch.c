/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "emscripten_fetch.h"
#include "emscripten_fetch_public.h"

#include "resindex.h"
#include "rwops/rwops_dummy.h"

#include <dirent.h>
#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <sys/stat.h>
#include <unistd.h>

#define CACHE_PATH "/persistent/res-cache"
#define MAX_PREFETCH_REQUESTS 4

#define MAKE_CACHED_PATH(_var_name, _content_id) \
	char _var_name[sizeof(CACHE_PATH) + strlen(_content_id) + 1]; \
	snprintf(_var_name, sizeof(_var_name), CACHE_PATH "/%s", (_content_id))

#define MAKE_FETCH_URL(_var_name, _content_id) \
	char _var_name[sizeof(TAISEI_BUILDCONF_DATA_PATH) + strlen(_content_id) + 1]; \
	snprintf(_var_name, sizeof(_var_name), TAISEI_BUILDCONF_DATA_PATH "/%s", (_content_id))

#define FETCH_CONTENT_ID(_fetch) \
	((_fetch)->url + sizeof(TAISEI_BUILDCONF_DATA_PATH))

typedef struct FetchFSContext {
	VFSResIndexFSContext resindex_ctx;
	ht_str2ptr_t requests;
	uint prefetch_iter;
} FetchFSContext;

static void fetch_begin_request(FetchFSContext *ctx, const char *url, const char *content_id);

EM_JS(void, update_dl_status, (int done, int total), {
	Module["setStatus"](
		done >= total
			? ""
			: "Prefetching resources… (" + done + "/" + total + ")",
	true);
});

static void prefetch_next(FetchFSContext *ctx) {
	while(ctx->requests.num_elements_occupied < MAX_PREFETCH_REQUESTS) {
		update_dl_status(ctx->prefetch_iter, resindex_num_file_entries());

		if(ctx->prefetch_iter >= resindex_num_file_entries()) {
			break;
		}

		const char *content_id = resindex_get_file_entry(ctx->prefetch_iter++)->content_id;
		MAKE_CACHED_PATH(cached_path, content_id);

		struct stat statbuf;
		if(stat(cached_path, &statbuf) >= 0) {
			continue;
		}

		MAKE_FETCH_URL(url, content_id);
		fetch_begin_request(ctx, url, content_id);
	}
}

static void fetch_finalize(emscripten_fetch_t *fetch) {
	FetchFSContext *ctx = fetch->userData;
	assert(ht_get(&ctx->requests, fetch->url, NULL) == fetch);
	ht_unset(&ctx->requests, fetch->url);
	prefetch_next(ctx);
}

static void fetch_onsuccess(emscripten_fetch_t *fetch) {
	MAKE_CACHED_PATH(path, FETCH_CONTENT_ID(fetch));

	if(!SDL_SaveFile(path, fetch->data, fetch->numBytes)) {
		log_sdl_error(LOG_ERROR, "SDL_SaveFile");
	} else {
		log_info("Cached %s as %s", fetch->url, path);
	}

	fetch_finalize(fetch);
	emscripten_fetch_close(fetch);
}

static void fetch_onerror(emscripten_fetch_t *fetch) {
	log_error("Failed to download %s (status %i: %s)",
		fetch->url, fetch->status, fetch->statusText
	);

	fetch_finalize(fetch);
	emscripten_fetch_close(fetch);
}

static void fetch_begin_request(FetchFSContext *ctx, const char *url, const char *content_id) {
	if(ht_lookup(&ctx->requests, url, NULL)) {
		return;
	}

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strlcpy(attr.requestMethod, "GET", sizeof(attr.requestMethod));
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.destinationPath = content_id;
	attr.overriddenMimeType = "application/octet-stream";
	attr.userData = ctx;
	attr.onsuccess = fetch_onsuccess;
	attr.onerror = fetch_onerror;

	ht_set(&ctx->requests, url, emscripten_fetch(&attr, url));
	log_info("Downloading %s", url);
}

static SDL_IOStream *vfs_fetch_open(VFSResIndexFSContext *resindex_ctx, const char *content_id, VFSOpenMode mode) {
	assert(!(mode & VFS_MODE_WRITE));

	FetchFSContext *ctx = UNION_CAST(VFSResIndexFSContext*, FetchFSContext*, resindex_ctx);

	MAKE_CACHED_PATH(cached_path, content_id);

	for(;;) {
		SDL_IOStream *cached_io = SDL_IOFromFile(cached_path, "r");

		if(cached_io) {
			return cached_io;
		}

		MAKE_FETCH_URL(url, content_id);
		fetch_begin_request(ctx, url, content_id);

		// cursed
		while(ht_lookup(&ctx->requests, url, NULL)) {
			emscripten_sleep(10);
		}
	}
}

static void vfs_fetch_free(VFSResIndexFSContext *resindex_ctx) {
	FetchFSContext *ctx = UNION_CAST(VFSResIndexFSContext*, FetchFSContext*, resindex_ctx);
	ht_destroy(&ctx->requests);
	mem_free(ctx);
}

static void evict_stale_cache(void) {
	ht_str2int_t idset;
	ht_create(&idset);

	int num_files = resindex_num_file_entries();
	for(int i = 0; i < num_files; ++i) {
		const RIdxFileEntry *fentry = NOT_NULL(resindex_get_file_entry(i));
		ht_set(&idset, fentry->content_id, 1);
	}

	char path[PATH_MAX] = CACHE_PATH "/";
	char *fname = path + sizeof(CACHE_PATH);

	DIR *dp = opendir(CACHE_PATH);

	for(struct dirent *d; (d = readdir(dp));) {
		if(*d->d_name == '.') {
			continue;
		}

		if(!ht_lookup(&idset, d->d_name, NULL)) {
			strlcpy(fname, d->d_name, sizeof(path) - sizeof(CACHE_PATH));
			log_info("Removing stale cache entry %s", path);
			unlink(path);
		}
	}

	closedir(dp);
	ht_destroy(&idset);
}

VFSNode *vfs_fetch_create(void) {
	auto ctx = ALLOC(FetchFSContext, {
		.resindex_ctx.procs = {
			.open = vfs_fetch_open,
			.free = vfs_fetch_free,
		},
	});
	ht_create(&ctx->requests);
	mkdir(CACHE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	VFSNode *n = vfs_resindex_create(&ctx->resindex_ctx);
	evict_stale_cache();
	prefetch_next(ctx);
	return n;
}

bool vfs_mount_fetchfs(const char *mountpoint) {
	return vfs_mount_or_decref(vfs_root, mountpoint, vfs_fetch_create());
}
