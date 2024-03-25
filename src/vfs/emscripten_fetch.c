/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
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

#define FETCH_CONTENT_ID(_fetch) \
	((_fetch)->url + sizeof(TAISEI_BUILDCONF_DATA_PATH))

typedef struct FetchFSContext {
	VFSResIndexFSContext resindex_ctx;
	ht_str2ptr_t requests;
	uint prefetch_iter;
} FetchFSContext;

static emscripten_fetch_t *fetch_begin_request(FetchFSContext *ctx, const char *content_id);

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

		fetch_begin_request(ctx, content_id);
	}
}

static void fetch_finalize(emscripten_fetch_t *fetch) {
	FetchFSContext *ctx = fetch->userData;
	assert(ht_get(&ctx->requests, fetch->url, NULL) == fetch);
	ht_unset(&ctx->requests, fetch->url);
	prefetch_next(ctx);
}

static void fetch_onsuccess(emscripten_fetch_t *fetch) {
	fetch_finalize(fetch);

	MAKE_CACHED_PATH(path, FETCH_CONTENT_ID(fetch));

	SDL_IOStream *rw = SDL_IOFromFile(path, "w");
	SDL_WriteIO(rw, fetch->data, fetch->numBytes);
	SDL_CloseIO(rw);

	log_info("Cached %s as %s", fetch->url, path);
}

static void fetch_onerror(emscripten_fetch_t *fetch) {
	log_error(
		"Failed to download %s (status %i: %s)",
		fetch->url, fetch->status, fetch->statusText
	);
	fetch_finalize(fetch);
}

static emscripten_fetch_t *fetch_begin_request(FetchFSContext *ctx, const char *content_id) {
	emscripten_fetch_t *fetch;

	char url[sizeof(TAISEI_BUILDCONF_DATA_PATH) + strlen(content_id) + 1];
	snprintf(url, sizeof(url), TAISEI_BUILDCONF_DATA_PATH "/%s", content_id);

	if(ht_lookup(&ctx->requests, url, (void**)&fetch)) {
		return fetch;
	}

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.destinationPath = content_id;
	attr.overriddenMimeType = "application/octet-stream";
	attr.userData = ctx;
	attr.onsuccess = fetch_onsuccess;
	attr.onerror = fetch_onerror;

	fetch = emscripten_fetch(&attr, url);
	ht_set(&ctx->requests, url, fetch);

	log_info("Downloading %s", url);
	return fetch;
}

static void fetch_wait(emscripten_fetch_t *fetch) {
	// cursed
	while(fetch->readyState < 4 /* DONE */) {
		emscripten_sleep(10);
	}
}

static int fetch_rwops_close(SDL_IOStream *rw) {
	// HACK: mega-jank, heavily relies on rwops_dummy implementation details
	int r = SDL_CloseIO(rw->hidden.unknown.data1);
	emscripten_fetch_close(rw->hidden.unknown.data2);
	return r;
}

static SDL_IOStream *vfs_fetch_open(VFSResIndexFSContext *resindex_ctx, const char *content_id, VFSOpenMode mode) {
	assert(!(mode & VFS_MODE_WRITE));

	FetchFSContext *ctx = UNION_CAST(VFSResIndexFSContext*, FetchFSContext*, resindex_ctx);
	emscripten_fetch_t *fetch;

	MAKE_CACHED_PATH(cached_path, content_id);

	SDL_IOStream *cached_rw = SDL_IOFromFile(cached_path, "r");
	if(cached_rw) {
		return cached_rw;
	}

	for(;;) {
		fetch = fetch_begin_request(ctx, content_id);
		fetch_wait(fetch);

		if(fetch->status == 200) {
			break;
		}

		emscripten_fetch_close(fetch);
	}

	SDL_IOStream *memrw = NOT_NULL(SDL_IOFromConstMem(fetch->data, fetch->numBytes));
	SDL_IOStream *wraprw = SDL_RWWrapDummy(memrw, true);
	wraprw->close = fetch_rwops_close;
	wraprw->hidden.unknown.data2 = fetch;

	return wraprw;
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
			strcpy(fname, d->d_name);
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
