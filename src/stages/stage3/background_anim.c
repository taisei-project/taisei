/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "background_anim.h"
#include "draw.h"

#include "global.h"
#include "stageutils.h"

// TODO

TASK(animate_bg) {
	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage3_bg_init_fullstage(void) {
	INVOKE_TASK(animate_bg);
}

void stage3_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg);
}
