/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"

Global global;

void init_global(CLIAction *cli) {
	memset(&global, 0, sizeof(global));

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

static SDL_atomic_t quitting;

void taisei_quit(void) {
	if(global.is_kiosk_mode && env_get("TAISEI_KIOSK_PREVENT_QUIT", true)) {
		log_info("Running in kiosk mode; exit request ignored");
		return;
	}

	if(SDL_AtomicCAS(&quitting, 0, 1)) {
		log_info("Exit requested");
	}
}

bool taisei_quit_requested(void) {
	return SDL_AtomicGet(&quitting);
}

void taisei_commit_persistent_data(void) {
	config_save();
	progress_save();
	vfs_sync(VFS_SYNC_STORE, NO_CALLCHAIN);
}
