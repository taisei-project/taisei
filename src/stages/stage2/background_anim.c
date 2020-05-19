/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "background_anim.h"

#include "stageutils.h"
#include "coroutine.h"

TASK(animate_bg_fullstage, NO_ARGS) {
	stage_3d_context.cx[2] = 1000;
	stage_3d_context.cx[0] = -850;
	stage_3d_context.crot[0] = 60;
	stage_3d_context.crot[2] = -90;
	stage_3d_context.cv[0] = 9;

	for(;;) {
		YIELD;
		fapproach_p(&stage_3d_context.cv[0], 0.0f, 0.05f);
		fapproach_p(&stage_3d_context.cv[1], 9.0f, 0.05f);
		stage_3d_context.crot[2] += fmin(0.5f, -stage_3d_context.crot[2] * 0.02f);
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_fullstage(void) {
	INVOKE_TASK(animate_bg_fullstage);
}

TASK(animate_bg_spellpractice, NO_ARGS) {
	stage_3d_context.cx[2] = 1000;
	stage_3d_context.cx[0] = -44.5;
	stage_3d_context.crot[0] = 60;
	stage_3d_context.cv[1] = 9;

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg_spellpractice);
}
