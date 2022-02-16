/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "filewatch.h"
#include "util.h"
#include "events.h"

#include <sys/inotify.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define INVALID_WD (-1)

#define WATCHED_EVENTS ( \
	IN_CLOSE_WRITE     | \
	IN_DELETE_SELF     | \
	IN_MODIFY          | \
	IN_MOVE_SELF       | \
0)

// FIXME: This doesn't detect removals of files with more than 1 hard link.
// IM_DELETE_SELF is only emitted when the link count drops to 0.
// Otherwise, IN_ATTRIB is emitted, indicating that the number of hard links has changed.
// Unfortunately, IN_ATTRIB is also emitted for *any* attribute change on the file, such as
// permissions or even access time, and there is no good way to distinguish a hard link removal from
// anything else. In principle, we could test if the path we're interested in monitoring is still
// accessible after an IN_ATTRIB event. However, that would require us to actually track the paths // here, complicating the code. Note that it is possible for multiple watched paths to resolve to
// the same file/watch descriptor. In the current implementation, FileWatch instances correspond
// 1-to-1 to watch descriptors, not paths.
#define DELETION_EVENTS ( \
	IN_DELETE_SELF      | \
	IN_IGNORED          | \
	IN_MOVE_SELF        | \
0)

#define FW (*_fw_globals)

#define EVENTS_BUF_SIZE 4096

struct FileWatch {
	int wd;
	SDL_atomic_t refs;
	bool updated;
	bool deleted;
};

struct {
	int inotify;
	ht_int2ptr_ts_t wd_to_watch;
	SDL_mutex *modify_mtx;
	DYNAMIC_ARRAY(FileWatch*) deactivate_list;
} *_fw_globals;

static bool filewatch_frame_event(SDL_Event *e, void *a);

void filewatch_init(void) {
	assert(_fw_globals == NULL);

	int inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

	if(UNLIKELY(inotify < 0)) {
		log_error("Failed to initialize inotify: %s", strerror(errno));
		return;
	}

	_fw_globals = calloc(1, sizeof(*_fw_globals));
	FW.inotify = inotify;
	ht_create(&FW.wd_to_watch);

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
		ht_destroy(&FW.wd_to_watch);
		dynarray_free_data(&FW.deactivate_list);
		SDL_DestroyMutex(FW.modify_mtx);

		free(_fw_globals);
		_fw_globals = NULL;
	}
}

FileWatch *filewatch_watch(const char *syspath) {
	if(!&FW) {
		log_error("Subsystem not initialized");
		return NULL;
	}

	SDL_LockMutex(FW.modify_mtx);
	int wd = inotify_add_watch(FW.inotify, syspath, WATCHED_EVENTS);

	if(UNLIKELY(wd == INVALID_WD)) {
		SDL_UnlockMutex(FW.modify_mtx);
		log_error("Failed to watch '%s': %s", syspath, strerror(errno));
		return NULL;
	}

	FileWatch *w;

	if(ht_lookup(&FW.wd_to_watch, wd, (void**)&w)) {
		assert(w->wd == wd);
	} else {
		w = calloc(1, sizeof(*w));
		w->wd = wd;
		ht_set(&FW.wd_to_watch, wd, w);
	}

	SDL_AtomicIncRef(&w->refs);
	SDL_UnlockMutex(FW.modify_mtx);

	return NOT_NULL(w);
}

static void filewatch_deactivate(FileWatch *watch) {
	SDL_LockMutex(FW.modify_mtx);

	if(watch->wd != INVALID_WD) {
		assert(ht_get(&FW.wd_to_watch, watch->wd, NULL) == watch);

		if(UNLIKELY(inotify_rm_watch(FW.inotify, watch->wd) == -1)) {
			log_warn("Failed to remove inotify watch: %s", strerror(errno));
		}

		ht_unset(&FW.wd_to_watch, watch->wd);
		watch->wd = INVALID_WD;
	}

	SDL_UnlockMutex(FW.modify_mtx);
}

void filewatch_unwatch(FileWatch *watch) {
	if(SDL_AtomicDecRef(&watch->refs)) {
		filewatch_deactivate(watch);
		free(watch);
	}
}

static void filewatch_process_events(ssize_t bufsize, char buf[bufsize]) {
	struct inotify_event *e;

	for(ssize_t i = 0; i < bufsize;) {
		e = CASTPTR_ASSUME_ALIGNED(buf + i, struct inotify_event);
		FileWatch *w = ht_get(&FW.wd_to_watch, e->wd, NULL);

		if(w == NULL) {
			goto skip;
		}

		if(e->mask & DELETION_EVENTS) {
			w->deleted = true;
		} else {
			w->updated = true;
		}
	skip:
		i += sizeof(*e) + e->len;
	}
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

		filewatch_process_events(r, buf);
	}

	ht_lock(&FW.wd_to_watch);
	ht_int2ptr_ts_iter_t iter;
	ht_iter_begin(&FW.wd_to_watch, &iter);

	for(;iter.has_data; ht_iter_next(&iter)) {
		FileWatch *w = NOT_NULL(iter.value);

		if(w->deleted) {
			events_emit(TE_FILEWATCH, FILEWATCH_FILE_DELETED, w, NULL);
			w->deleted = w->updated = false;
			// defer filewatch_deactivate(w) because it modifies this hashtable
			*dynarray_append(&FW.deactivate_list) = w;
		} else if(w->updated) {
			events_emit(TE_FILEWATCH, FILEWATCH_FILE_UPDATED, w, NULL);
			w->updated = false;
		}
	}

	ht_iter_end(&iter);
	ht_unlock(&FW.wd_to_watch);

	dynarray_foreach_elem(&FW.deactivate_list, FileWatch **w, {
		filewatch_deactivate(*w);
	});

	FW.deactivate_list.num_elements = 0;
	SDL_UnlockMutex(FW.modify_mtx);
}

static bool filewatch_frame_event(SDL_Event *e, void *a) {
	filewatch_process();
	return false;
}
