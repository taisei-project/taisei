/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#ifndef TAISEI_BUILDCONF_HAVE_POSIX
#error Stage hot reloading is only supported on POSIX systems
#endif

#include "dynstage.h"
#include "events.h"
#include "filewatch/filewatch.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define HT_SUFFIX                      fwatchset
#define HT_KEY_TYPE                    FileWatch*
#define HT_VALUE_TYPE                  struct ht_empty
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uintptr_t)(key))
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (FileWatch*)(src))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "i"
#define HT_VALUE_PRINTABLE(val)        (1)
#define HT_KEY_CONST
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

typedef void *stageslib_t;

// in frames
#define RECOMPILE_DELAY 6
#define RESCAN_DELAY 6
#define LIBRARY_BUMP_DELAY 6

static struct {
	stageslib_t lib;
	StagesExports *exports;
	FileWatch *lib_watch;
	uint32_t lib_generation;
	uint32_t src_generation;
	ht_fwatchset_t src_watches;
	bool monitoring_initialized;
	int recompile_delay;
	int rescan_delay;
	int lib_gen_bump_delay;
} dynstage;

static stageslib_t dynstage_dlopen(void) {
	int attempts = 7;
	int delay = 10;

	for(;;) {
		stageslib_t lib = dlopen(TAISEI_BUILDCONF_DYNSTAGE_LIB, RTLD_NOW | RTLD_LOCAL);

		if(LIKELY(lib != NULL)) {
			return lib;
		}

		if(--attempts) {
			log_error("Failed to load stages library (%i attempt%s left): %s", attempts, attempts > 1? "s" : "", dlerror());
			SDL_Delay(delay);
			delay *= 2;
		} else {
			break;
		}
	}

	log_fatal("Failed to load stages library: %s", dlerror());
}

static void dynstage_bump_lib_generation(void) {
	// Delay reporting library update to avoid trying to load a partially written file
	dynstage.lib_gen_bump_delay = imax(dynstage.lib_gen_bump_delay, LIBRARY_BUMP_DELAY);
}

static void dynstage_bump_src_generation(bool deleted) {
	++dynstage.src_generation;
	dynstage.recompile_delay = imax(dynstage.recompile_delay, RECOMPILE_DELAY);

	if(deleted) {
		// Some text editors may delete the original file and replace it when saving.
		// In case that happens, rescan shortly after detecting a delete.
		// Not very efficient, but simple since we don't have to track file names.
		dynstage.rescan_delay = imax(dynstage.rescan_delay, RESCAN_DELAY);
	}

	log_debug("%i", dynstage.src_generation);
}

static void unwatch_sources(void);
static void watch_sources(void);
static void recompile(void);

static bool dynstage_frame_event(SDL_Event *e, void *ctx) {
	if(!dynstage.lib_watch) {
		dynstage.lib_watch = filewatch_watch(TAISEI_BUILDCONF_DYNSTAGE_LIB);

		if(dynstage.lib_watch) {
			log_debug("Created watch for %s", TAISEI_BUILDCONF_DYNSTAGE_LIB);
			dynstage_bump_lib_generation();
		}
	}

	if(dynstage.rescan_delay > 0 && --dynstage.rescan_delay == 0) {
		unwatch_sources();
		watch_sources();
	}

	if(dynstage.recompile_delay > 0 && --dynstage.recompile_delay == 0) {
		recompile();
	}

	if(dynstage.lib_gen_bump_delay > 0 && --dynstage.lib_gen_bump_delay == 0) {
		++dynstage.lib_generation;
		log_debug("Bumped library generation: %i", dynstage.lib_generation);
	}

	return false;
}

static bool is_src_watch(FileWatch *w) {
	return ht_fwatchset_lookup(&dynstage.src_watches, w, NULL);
}

static bool dynstage_filewatch_event(SDL_Event *e, void *ctx) {
	FileWatch *watch = NOT_NULL(e->user.data1);
	FileWatchEvent fevent = e->user.code;

	if(watch == dynstage.lib_watch) {
		switch(fevent) {
			case FILEWATCH_FILE_UPDATED:
				log_debug("Library updated");
				dynstage_bump_lib_generation();
				break;

			case FILEWATCH_FILE_DELETED:
				log_debug("Library deleted");
				break;
		}

		return false;
	}

	if(is_src_watch(watch)) {
		dynstage_bump_src_generation(fevent == FILEWATCH_FILE_DELETED);
	}

	return false;
}

static void dynstage_sigchld(int sig, siginfo_t *sinfo, void *ctx) {
	attr_unused pid_t cpid;
	int status;

	while((cpid = waitpid(-1, &status, WNOHANG)) > 0) {
		log_debug("Process %i terminated with status %i", cpid, status);
	}
}

void dynstage_init(void) {
	struct sigaction sa = {
		.sa_sigaction = dynstage_sigchld,
		.sa_flags = SA_SIGINFO,
	};

	if(sigaction(SIGCHLD, &sa, NULL) == -1) {
		log_fatal("sigaction() failed: %s", strerror(errno));
	}

	dynstage_reload_library();
}

static void unwatch_sources(void) {
	ht_fwatchset_iter_t iter;
	ht_fwatchset_iter_begin(&dynstage.src_watches, &iter);

	for(;iter.has_data; ht_fwatchset_iter_next(&iter)) {
		filewatch_unwatch(iter.key);
	}

	ht_fwatchset_iter_end(&iter);
	ht_fwatchset_unset_all(&dynstage.src_watches);
}

static void watch_source_file(const char *path) {
	FileWatch *w = filewatch_watch(path);

	if(!w) {
		return;
	}

	if(is_src_watch(w)) {
		// filewatches are reference counted; keep only 1 reference
		filewatch_unwatch(w);
	} else {
		ht_fwatchset_set(&dynstage.src_watches, w, HT_EMPTY);
	}
}

static int watch_source_dir(const char *path) {
	DIR *d = opendir(path);

	if(!d) {
		return errno;
	}

	size_t pathlen = strlen(path);

	struct dirent *e;
	while((e = readdir(d))) {
		const char *n = e->d_name;

		if(!*n || !strcmp(n, ".") || !strcmp(n, "..")) {
			continue;
		}

		size_t nlen = strlen(n);
		char pathbuf[pathlen + nlen + 2];
		memcpy(pathbuf, path, pathlen);
		pathbuf[pathlen] = '/';
		memcpy(pathbuf + pathlen + 1, n, nlen + 1);

		log_debug("%s", pathbuf);

		if(watch_source_dir(pathbuf) == ENOTDIR) {
			watch_source_file(pathbuf);
		}
	}

	closedir(d);
	return 0;
}

static void watch_sources(void) {
	watch_source_dir(TAISEI_BUILDCONF_DYNSTAGE_SRCROOT);
}

void dynstage_init_monitoring(void) {
	events_register_handler(&(EventHandler) {
		.proc = dynstage_frame_event,
		.priority = EPRIO_SYSTEM,
		.event_type = MAKE_TAISEI_EVENT(TE_FRAME),
	});

	events_register_handler(&(EventHandler) {
		.proc = dynstage_filewatch_event,
		.priority = EPRIO_SYSTEM,
		.event_type = MAKE_TAISEI_EVENT(TE_FILEWATCH),
	});

	ht_fwatchset_create(&dynstage.src_watches);
	watch_sources();

	dynstage.monitoring_initialized = true;
}

static void recompile(void) {
	pid_t cpid = fork();

	if(UNLIKELY(cpid < 0)) {
		log_error("fork() failed: %s", strerror(errno));
		return;
	}

	if(cpid == 0) {
		static char *const cmd[] = TAISEI_BUILDCONF_DYNSTAGE_REBUILD_CMD;
		execvp(cmd[0], cmd);
		abort();
	}

	log_debug("Child pid: %ji", (intmax_t)cpid);
}

void dynstage_shutdown(void) {
	if(dynstage.monitoring_initialized) {
		events_unregister_handler(dynstage_frame_event);
		events_unregister_handler(dynstage_filewatch_event);

		if(dynstage.lib_watch) {
			filewatch_unwatch(dynstage.lib_watch);
		}

		unwatch_sources();
		ht_fwatchset_destroy(&dynstage.src_watches);
	}

	dlclose(dynstage.lib);
	sigaction(SIGCHLD, &(struct sigaction) { .sa_handler = SIG_DFL }, NULL);
	memset(&dynstage, 0, sizeof(dynstage));
}

uint32_t dynstage_get_generation(void) {
	return dynstage.lib_generation;
}

bool dynstage_reload_library(void) {
	if(dynstage.lib) {
		// In-flight async log messages may still have pointers to static strings inside the old lib
		// Wait for them to finish processing so it's safe to unload
		log_sync(false);

		dlclose(dynstage.lib);
	}

	dynstage.exports = dlsym((dynstage.lib = dynstage_dlopen()), "stages_exports");

	if(UNLIKELY(dynstage.exports == NULL)) {
		log_fatal("No stages_exports symbol in stages library");
	}

	log_info("Reloaded stages module");
	return true;
}

StagesExports *dynstage_get_exports(void) {
	return dynstage.exports;
}
