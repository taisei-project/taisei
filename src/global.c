/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include "video.h"
#include <time.h>
#include <stdio.h>
#include <png.h>
#include "paths/native.h"
#include "resource/resource.h"
#include "taisei_err.h"
#include "replay.h"

Global global;

void init_global(void) {
	memset(&global, 0, sizeof(global));

	tsrand_init(&global.rand_game, time(0));
	tsrand_init(&global.rand_visual, time(0));

	tsrand_switch(&global.rand_visual);

	memset(&resources, 0, sizeof(Resources));

	memset(&global.replay, 0, sizeof(Replay));
	global.replaymode = REPLAY_RECORD;

	if(global.frameskip = getenvint("TAISEI_SANIC")) {
		if(global.frameskip < 0) {
			global.frameskip = INT_MAX;
		}

		warnx("FPS limiter disabled by environment. Gotta go fast! (frameskip = %i)", global.frameskip);
	}
}
