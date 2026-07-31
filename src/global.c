/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "global.h"

#include "taskmanager.h"
#include "util/env.h"
#include "gamepad.h"

Global global;

void init_global(CLIAction *cli) {
	global = (typeof(global)) {};

	rng_init(&global.rand_game, time(0));
	rng_init(&global.rand_visual, time(0));
	rng_make_active(&global.rand_visual);

	global.frameskip = cli->frameskip;

	if(cli->type == CLI_VerifyReplay) {
		global.is_headless = true;
		global.is_replay_verification = true;
		global.frameskip = 1;
	} else if(global.frameskip) {
		log_warn("FPS limiter disabled. Gotta go fast! (frameskip = %i)", global.frameskip);
	}

	global.is_kiosk_mode = env_get("TAISEI_KIOSK", false);

	if(global.is_kiosk_mode) {
		SDL_SetHintWithPriority(SDL_HINT_NO_SIGNAL_HANDLERS, 0, SDL_HINT_DEFAULT);
	}

	fpscounter_reset(&global.fps.logic);
	fpscounter_reset(&global.fps.render);
	fpscounter_reset(&global.fps.busy);
}

// Inputdevice-agnostic method of checking whether a game control is pressed.
// ALWAYS use this instead of SDL_GetKeyState if you need it.
// XXX: Move this somewhere?
bool gamekeypressed(KeyIndex key) {
	return SDL_GetKeyboardState(NULL)[config_get_int(KEYIDX_TO_CFGIDX(key))] || gamepad_game_key_pressed(key);
}

static SDL_AtomicInt quitting;

static bool taisei_is_quit_forbidden(void) {
	return global.is_kiosk_mode && env_get("TAISEI_KIOSK_PREVENT_QUIT", true);
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

	if(SDL_CompareAndSwapAtomicInt(&quitting, 0, 1)) {
		log_info("Exit requested");
	}
}

bool taisei_quit_requested(void) {
	return SDL_GetAtomicInt(&quitting);
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

void taisei_commit_persistent_data(void) {
	if(!SDL_CompareAndSwapAtomicInt(&syncing_data, 0, 1)) {
		log_warn("Commit already in progress");
		return;
	}

	log_debug("Begin committing persistent data");

	task_detach(taskmgr_global_submit((TaskParams) {
		.callback = sync_task,
	}));
}
