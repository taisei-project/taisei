/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

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

	if(global.frameskip) {
		log_warn("FPS limiter disabled. Gotta go fast! (frameskip = %i)", global.frameskip);
	}
}
