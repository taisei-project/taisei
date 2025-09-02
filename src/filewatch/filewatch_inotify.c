/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "filewatch.h"

#include "events.h"
#include "hashtable.h"
#include "list.h"
#include "vfs/syspath_public.h"

#include <alloca.h>
#include <errno.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <unistd.h>

// #define FW_DEBUG

#ifdef FW_DEBUG
	#undef FW_DEBUG
	#define FW_DEBUG(...) log_debug(__VA_ARGS__)
	#define IF_FW_DEBUG(...) __VA_ARGS__
#else
	#define FW_DEBUG(...) ((void)0)
	#define IF_FW_DEBUG(...)
#endif

#ifndef IN_MASK_CREATE
// libinotify-kqueue doesn't have this
#define IN_MASK_CREATE (0)
#endif

#define WATCH_FLAGS ( \
	IN_CLOSE_WRITE      | \
	IN_CREATE           | \
	IN_DELETE           | \
	IN_DELETE_SELF      | \
	IN_MODIFY           | \
	IN_MOVED_FROM       | \
	IN_MOVED_TO         | \
	IN_MOVE_SELF        | \
	                      \
	IN_EXCL_UNLINK      | \
	IN_ONLYDIR          | \
0)

#define INVALID_WD (-1)

#define FW (*_fw_globals)

#define EVENTS_BUF_SIZE 4096

typedef struct DirWatch DirWatch;

struct FileWatch {
	LIST_INTERFACE(FileWatch);
	char *filename;
	DirWatch *dw;
	bool updated;
	bool deleted;
};

struct DirWatch {
	LIST_INTERFACE(DirWatch);
	FileWatch *filewatch_list;
	char *path;
	int wd;
};

typedef struct WDRecord {
	DirWatch *dirwatch_list;
	int32_t refs;
} WDRecord;

#define HT_SUFFIX                      int2wdrecord
#define HT_KEY_TYPE                    int32_t
#define HT_VALUE_TYPE                  WDRecord
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint32((uint32_t)(key))
#define HT_KEY_FMT                     PRIi32
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "i"
#define HT_VALUE_PRINTABLE(val)        ((val).wd)
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

#define HT_SUFFIX                      filewatchset
#define HT_KEY_TYPE                    FileWatch*
#define HT_VALUE_TYPE                  struct ht_empty
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uint64_t)(key))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          ((void*)(key))
#define HT_VALUE_FMT                   "s"
#define HT_VALUE_PRINTABLE(val)        "(empty)"
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

struct {
	int inotify;
	ht_int2wdrecord_t wd_records;
	ht_str2ptr_t dirpath_to_dirwatch;
	ht_filewatchset_t updated_watches;
	SDL_Mutex *modify_mtx;
} *_fw_globals;

static WDRecord *wdrecord_get(int wd, bool create) {
	WDRecord *r = NULL;

	if(!ht_int2wdrecord_get_ptr_unsafe(&FW.wd_records, wd, &r, create)) {
		if(create) {
			*r = (WDRecord) {};
		}
	}

	return r;
}

static void wdrecord_delete(int wd) {
	ht_int2wdrecord_unset(&FW.wd_records, wd);
}

static int wdrecord_incref(WDRecord *r) {
	return r->refs++;
}

static int wdrecord_decref(WDRecord *r) {
	int32_t rc = --r->refs;
	assert(rc >= 0);
	return rc;
}

static DirWatch *dirwatch_get(const char *path, bool create) {
	DirWatch *dw = ht_get(&FW.dirpath_to_dirwatch, path, NULL);

	if(dw || !create) {
		if(dw) {
			assert(dw->path != NULL);
			assert(!strcmp(dw->path, path));
		}

		return dw;
	}

	int wd = inotify_add_watch(FW.inotify, path, WATCH_FLAGS);

	if(UNLIKELY(wd == INVALID_WD)) {
		log_error("%s: %s", path, strerror(errno));
		return NULL;
	}

	dw = ALLOC(typeof(*dw));
	dw->path = mem_strdup(path);
	dw->wd = wd;

	WDRecord *wdrec = NOT_NULL(wdrecord_get(wd, true));
	list_push(&wdrec->dirwatch_list, dw);
	wdrecord_incref(wdrec);

	ht_set(&FW.dirpath_to_dirwatch, path, dw);

	return dw;
}

static void dirwatch_delete(DirWatch *dw) {
	assert(dw->filewatch_list == NULL);

	WDRecord *wdrec = NOT_NULL(wdrecord_get(dw->wd, false));
	list_unlink(&wdrec->dirwatch_list, dw);
	ht_unset(&FW.dirpath_to_dirwatch, dw->path);
	mem_free(dw->path);

	if(wdrecord_decref(wdrec) == 0) {
		inotify_rm_watch(FW.inotify, dw->wd);
		wdrecord_delete(dw->wd);
	}

	mem_free(dw);
}

static void dirwatch_invalidate(DirWatch *dw) {
	log_warn("%s: Unimplemented (FIXME)", dw->path);
}

static bool filewatch_frame_event(SDL_Event *e, void *a);

attr_unused
static void dump_events(StringBuffer *sbuf, uint events) {
	#define DELIM() do { \
		if(sbuf->pos != sbuf->start) { \
			strbuf_cat(sbuf, " | "); \
		} \
	} while(0)

	#define HANDLE(evt) do { \
		if(events & evt) { \
			DELIM(); \
			strbuf_cat(sbuf, #evt); \
			events &= ~evt; \
		} \
	} while(0)

	HANDLE(IN_ACCESS);
	HANDLE(IN_MODIFY);
	HANDLE(IN_ATTRIB);
	HANDLE(IN_CLOSE_WRITE);
	HANDLE(IN_CLOSE_NOWRITE);
	HANDLE(IN_OPEN);
	HANDLE(IN_MOVED_FROM);
	HANDLE(IN_MOVED_TO);
	HANDLE(IN_CREATE);
	HANDLE(IN_DELETE);
	HANDLE(IN_DELETE_SELF);
	HANDLE(IN_MOVE_SELF);

	HANDLE(IN_UNMOUNT);
	HANDLE(IN_Q_OVERFLOW);
	HANDLE(IN_IGNORED);

	HANDLE(IN_ONLYDIR);
	HANDLE(IN_DONT_FOLLOW);
	HANDLE(IN_EXCL_UNLINK);
	HANDLE(IN_MASK_CREATE);
	HANDLE(IN_MASK_ADD);
	HANDLE(IN_ISDIR);
	HANDLE(IN_ONESHOT);

	if(events) {
		DELIM();
		strbuf_printf(sbuf, "0x%08x", events);
	}

	#undef DELIM
	#undef HANDLE
}

void filewatch_init(void) {
	assert(_fw_globals == NULL);

	// NOTE: This is for libinotify-kqueue (macOS/*BSD), since it will possibly have to open a file
	// descriptor for every file inside a monitored directory. Linux is unaffected by this.
	struct rlimit lim;
	getrlimit(RLIMIT_NOFILE, &lim);
	lim.rlim_cur = lim.rlim_max;
	setrlimit(RLIMIT_NOFILE, &lim);

	int inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

	if(UNLIKELY(inotify < 0)) {
		log_error("Failed to initialize inotify: %s", strerror(errno));
		return;
	}

	_fw_globals = ALLOC(typeof(*_fw_globals));
	FW.inotify = inotify;
	ht_filewatchset_create(&FW.updated_watches);
	ht_int2wdrecord_create(&FW.wd_records);
	ht_create(&FW.dirpath_to_dirwatch);

	FW.modify_mtx = SDL_CreateMutex();

	if(UNLIKELY(FW.modify_mtx == NULL)) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
	}

	events_register_handler(&(EventHandler) {
		.proc = filewatch_frame_event,
		.priority = EPRIO_SYSTEM,
		.event_type = MAKE_TAISEI_EVENT(TE_FRAME),
	});
}

void filewatch_shutdown(void) {
	if(_fw_globals) {
		events_unregister_handler(filewatch_frame_event);

		close(FW.inotify);
		ht_filewatchset_destroy(&FW.updated_watches);
		ht_int2wdrecord_destroy(&FW.wd_records);
		ht_destroy(&FW.dirpath_to_dirwatch);
		SDL_DestroyMutex(FW.modify_mtx);

		mem_free(_fw_globals);
		_fw_globals = NULL;
	}
}

static bool split_path(char *pathbuf, const char **dirpath, const char **filename) {
	char *sep;
	size_t plen = strlen(pathbuf);

	// Strip trailing slash
	// TODO: maybe guarantee this in vfs_syspath_normalize?
	while(pathbuf[plen - 1] == '/') {
		pathbuf[plen - 1] = 0;

		if(--plen == 0) {
			return false;
		}
	}

	sep = memrchr(pathbuf, '/', plen);
	if(UNLIKELY(sep == NULL)) {
		return false;
	}

	*filename = sep + 1;

	if(UNLIKELY(sep == pathbuf)) {  // file in root directory?
		*dirpath = "/";
	} else {
		*sep = 0;
		*dirpath = pathbuf;
	}

	return true;
}

FileWatch *filewatch_watch(const char *syspath) {
	if(!&FW) {
		log_error("Subsystem not initialized");
		return NULL;
	}

	if(*syspath != '/') {
		log_error("%s: relative paths are not supported", syspath);
		return NULL;
	}

	// NOTE: we want to normalize the path, but not resolve symlinks,
	// so realpath(2) is not appropriate.
	char pathbuf[strlen(syspath) + 1];
	vfs_syspath_normalize(pathbuf, sizeof(pathbuf), syspath);

	const char *dirpath, *filename;
	if(UNLIKELY(!split_path(pathbuf, &dirpath, &filename))) {
		log_error("%s: can't split path", syspath);
		return NULL;
	}

	SDL_LockMutex(FW.modify_mtx);

	DirWatch *dw = dirwatch_get(dirpath, true);
	if(UNLIKELY(dw == NULL)) {
		SDL_UnlockMutex(FW.modify_mtx);
		log_error("%s: failed to watch parent directory", syspath);
		return NULL;
	}

	auto fw = list_push(&dw->filewatch_list, ALLOC(FileWatch, {
		.dw = dw,
		.filename = mem_strdup(filename),
	}));

	SDL_UnlockMutex(FW.modify_mtx);

	return fw;
}

void filewatch_unwatch(FileWatch *fw) {
	SDL_LockMutex(FW.modify_mtx);

	DirWatch *dw = NOT_NULL(fw->dw);
	list_unlink(&dw->filewatch_list, fw);

	mem_free(fw->filename);
	mem_free(fw);

	if(dw->filewatch_list == NULL) {
		dirwatch_delete(dw);
	}

	SDL_UnlockMutex(FW.modify_mtx);
}

static uint32_t filewatch_process_event(
	struct inotify_event *e
	IF_FW_DEBUG(, StringBuffer *sbuf)
) {
	if(UNLIKELY(e->wd == INVALID_WD)) {
		log_error("inotify queue overflow");
		return e->len;
	}

	WDRecord *rec = wdrecord_get(e->wd, false);

	IF_FW_DEBUG(
		strbuf_clear(sbuf);
		dump_events(sbuf, e->mask);
		FW_DEBUG("wd %i :: %s :: %s", e->wd, e->name, sbuf->start);
	)

	if(!rec) {
		return e->len;
	}

	for(DirWatch *dw = rec->dirwatch_list; dw; dw = dw->next) {
		if(dw->wd != e->wd) {
			continue;
		}

		FW_DEBUG("Match dir %s", dw->path);

		if(e->mask & IN_ISDIR) {
			if(e->mask & (IN_MOVE_SELF | IN_DELETE_SELF | IN_IGNORED)) {
				dirwatch_invalidate(dw);
			}

			continue;
		}

		for(FileWatch *fw = dw->filewatch_list; fw; fw = fw->next) {
			if(strcmp(fw->filename, e->name)) {
				FW_DEBUG("* %p %s != %s", fw, fw->filename, e->name);
				continue;
			}

			FW_DEBUG("Match file %s/%s", dw->path, fw->filename);

			if(e->mask & (IN_DELETE | IN_MOVED_FROM)) {
				fw->deleted = true;
				ht_filewatchset_set(&FW.updated_watches, fw, HT_EMPTY);
			} else if(e->mask & (IN_CREATE | IN_MOVED_TO)) {
				fw->deleted = false;
				fw->updated = true;
				ht_filewatchset_set(&FW.updated_watches, fw, HT_EMPTY);
			} else if(e->mask & (IN_CLOSE_WRITE | IN_MODIFY)) {
				fw->updated = true;
				ht_filewatchset_set(&FW.updated_watches, fw, HT_EMPTY);
			}
		}
	}

	return e->len;
}

static uint32_t filewatch_process_event_misaligned(
	size_t bufsize, char buf[bufsize]
	IF_FW_DEBUG(, StringBuffer *sbuf)
) {
	uint32_t sz;
	memcpy(&sz, buf + offsetof(struct inotify_event, len), sizeof(sz));
	assert(sz <= bufsize);

	struct inotify_event *e = alloca(sizeof(*e) + sz);
	memcpy(e, buf, sizeof(*e));

	filewatch_process_event(e  IF_FW_DEBUG(, sbuf));
	return sz;
}

static void filewatch_process_events(ssize_t bufsize, char buf[bufsize]) {
	IF_FW_DEBUG(
		StringBuffer sbuf = {};
	)

	for(ssize_t i = 0; i < bufsize;) {
		uintptr_t misalign = ((uintptr_t)(buf + i) & (alignof(struct inotify_event) - 1));
		uint32_t nlen;

		if(UNLIKELY(misalign)) {
			IF_FW_DEBUG(log_warn("inotify_event pointer %p misaligned by %u",
				buf + i, (uint)misalign));
			nlen = filewatch_process_event_misaligned(
				bufsize - i, buf + i
				IF_FW_DEBUG(, &sbuf)
			);
		} else {
			nlen = filewatch_process_event(
				CASTPTR_ASSUME_ALIGNED(buf + i, struct inotify_event)
				IF_FW_DEBUG(, &sbuf)
			);
		}

		i += sizeof(struct inotify_event) + nlen;
	}

	IF_FW_DEBUG(
		strbuf_free(&sbuf);
	)
}

static void filewatch_process(void) {
	SDL_LockMutex(FW.modify_mtx);
	alignas(alignof(struct inotify_event)) char buf[EVENTS_BUF_SIZE];

	for(;;) {
		ssize_t r = read(FW.inotify, buf, sizeof(buf));

		if(r == -1) {
			if(errno != EAGAIN) {
				log_error("%s", strerror(errno));
			}

			break;
		}

		FW_DEBUG("READ %ji", r);
		filewatch_process_events(r, buf);
	}

	if(FW.updated_watches.num_elements_occupied == 0) {
		SDL_UnlockMutex(FW.modify_mtx);
		return;
	}

	ht_filewatchset_iter_t iter;
	ht_filewatchset_iter_begin(&FW.updated_watches, &iter);

	for(;iter.has_data; ht_filewatchset_iter_next(&iter)) {
		FileWatch *fw = NOT_NULL(iter.key);

		if(fw->deleted) {
			events_emit(TE_FILEWATCH, FILEWATCH_FILE_DELETED, fw, NULL);
		} else if(fw->updated) {
			events_emit(TE_FILEWATCH, FILEWATCH_FILE_UPDATED, fw, NULL);
		}

		fw->deleted = fw->updated = false;
	}

	ht_filewatchset_iter_end(&iter);
	ht_filewatchset_unset_all(&FW.updated_watches);

	SDL_UnlockMutex(FW.modify_mtx);
}

static bool filewatch_frame_event(SDL_Event *e, void *a) {
	filewatch_process();
	return false;
}
