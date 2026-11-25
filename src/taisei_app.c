/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei_app.h"

#include "config.h"
#include "eventloop/eventloop.h"
#include "gamepad.h"
#include "log.h"
#include "progress.h"
#include "random.h"
#include "taskmanager.h"
#include "util/env.h"
#include "vfs/public.h"

#include <SDL3/SDL.h>

static struct {
	SDL_AtomicInt quitting;
	bool kiosk_mode;
	bool transient;
} app;

void taisei_init(CLIAction *cli) {
	app.kiosk_mode = env_get("TAISEI_KIOSK", false);

	if(app.kiosk_mode) {
		SDL_SetHintWithPriority(SDL_HINT_NO_SIGNAL_HANDLERS, 0, SDL_HINT_DEFAULT);
	}

	if(cli->type == CLI_VerifyReplay) {
		eventloop_set_headless(true);
		app.transient = true;
	} else if(cli->frameskip) {
		log_warn("FPS limiter disabled. Gotta go fast! (frameskip = %i)", cli->frameskip);
		eventloop_set_frameskip(cli->frameskip);
	}

	rng_init_default(time(0));
}

bool taisei_is_kiosk_mode(void) {
	return app.kiosk_mode;
}

static bool taisei_is_quit_forbidden(void) {
	return app.kiosk_mode && env_get("TAISEI_KIOSK_PREVENT_QUIT", true);
}

bool taisei_is_quit_hidden(void) {
#ifdef __EMSCRIPTEN__
	return true;
#else
	return taisei_is_quit_forbidden();
#endif
}

void taisei_quit(void) {
	if(taisei_is_quit_forbidden()) {
		log_info("Running in kiosk mode; exit request ignored");
		return;
	}

	if(SDL_CompareAndSwapAtomicInt(&app.quitting, 0, 1)) {
		log_info("Exit requested");
	}
}

bool taisei_quit_requested(void) {
	return SDL_GetAtomicInt(&app.quitting);
}

bool gamekeypressed(KeyIndex key) {
	return SDL_GetKeyboardState(NULL)[config_get_int(KEYIDX_TO_CFGIDX(key))] || gamepad_game_key_pressed(key);
}

static SDL_AtomicInt syncing_data;

static void sync_finished(CallChainResult ccr) {
	SDL_SetAtomicInt(&syncing_data, 0);
	log_debug("Finished committing persistent data");
}

static void *sync_task(void *arg) {
	config_save();
	progress_save();
	vfs_sync(VFS_SYNC_STORE, CALLCHAIN(sync_finished, NULL));
	return NULL;
}

void taisei_commit_persistent_data(bool async) {
	if(app.transient) {
		return;
	}

	if(!SDL_CompareAndSwapAtomicInt(&syncing_data, 0, 1)) {
		log_warn("Commit already in progress");
		return;
	}

	if(async) {
		log_debug("Begin committing persistent data");

		task_detach(taskmgr_global_submit((TaskParams) {
			.callback = sync_task,
		}));
	} else {
		sync_task(NULL);
	}
}
