/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "global.h"

#include "taisei_app.h"
#include "taskmanager.h"
#include "util/env.h"
#include "gamepad.h"

Global global;

void init_global(CLIAction *cli) {
	global = (typeof(global)) {};

	rng_init_default(time(0));

}

// Inputdevice-agnostic method of checking whether a game control is pressed.
// ALWAYS use this instead of SDL_GetKeyState if you need it.
// XXX: Move this somewhere?
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
	if(taisei_is_transient()) {
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
