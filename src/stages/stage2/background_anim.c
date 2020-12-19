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
#include "util/glm.h"

TASK(easing_animate, { float *val; float to; int time; glm_ease_t ease; }) {

    float from = *ARGS.val;
    float scale = ARGS.to - from;
    float ftime = ARGS.time;

    for(int t = 0; t < ARGS.time;t++) {
        YIELD;
        *ARGS.val = from + scale * ARGS.ease(t / ftime);
    }
}

TASK(animate_bg_fullstage, NO_ARGS) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = 8;
	cam->pos[0] = 10;
	cam->rot.v[0] = 50;
	cam->rot.v[2] = -90;

	real dur = 600;
	glm_ease_t func = glm_ease_quad_inout;
	INVOKE_TASK(easing_animate,&cam->rot.v[0], 40, dur, glm_ease_quad_out);
	INVOKE_TASK(easing_animate,&cam->rot.v[2], -2.5f, dur, glm_ease_quad_out);
	INVOKE_TASK(easing_animate,&cam->pos[0], 0, dur, func);
	INVOKE_TASK(easing_animate,&cam->vel[1], 0.05f, dur, func);

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_fullstage(void) {
	INVOKE_TASK(animate_bg_fullstage);
}

TASK(animate_bg_spellpractice, NO_ARGS) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = 8;
	cam->pos[0] = 0;
	cam->rot.v[0] = 40;
	cam->vel[1] = 0.05f;

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg_spellpractice);
}
