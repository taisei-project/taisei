/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"

Global global;

void init_global(CLIAction *cli) {
	memset(&global, 0, sizeof(global));

	tsrand_init(&global.rand_game, time(0));
	tsrand_init(&global.rand_visual, time(0));
	tsrand_switch(&global.rand_visual);

	memset(&resources, 0, sizeof(Resources));
	memset(&global.replay, 0, sizeof(Replay));

	global.replaymode = REPLAY_RECORD;
	global.frameskip = cli->frameskip;

	if(cli->type == CLI_VerifyReplay) {
		global.is_headless = true;
		global.is_replay_verification = true;
		global.frameskip = 1;
	} else if(global.frameskip) {
		log_warn("FPS limiter disabled. Gotta go fast! (frameskip = %i)", global.frameskip);
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
	if(SDL_AtomicCAS(&quitting, 0, 1)) {
		log_info("Exit requested");
	}
}

bool taisei_quit_requested(void) {
	return SDL_AtomicGet(&quitting);
}
