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

#include "stageutils.h"
#include "util/glm.h"

// merge with stage2 version?
TASK(easing_animate, { float *val; float to; int time; glm_ease_t ease; }) {
    float from = *ARGS.val;
    float scale = ARGS.to - from;
    float ftime = ARGS.time;

    for(int t = 0; t < ARGS.time;t++) {
        YIELD;
        *ARGS.val = from + scale * ARGS.ease(t / ftime);
    }
}

TASK(update_stage_3d, NO_ARGS) {
	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

TASK(animate_bg_fullstage, NO_ARGS) {
	Camera3D *cam = &stage_3d_context.cam;
	cam->far = 300;

	cam->pos[2] = -1.8;
	cam->pos[1] = -30;
	cam->rot.v[0] = 80;
	cam->vel[1] = 0.017;
	cam->vel[2] = 0.008;

	WAIT(400);
	INVOKE_TASK(easing_animate,&cam->vel[1], 0.013, 200, glm_ease_sine_inout);
	INVOKE_TASK(easing_animate,&cam->vel[2], 0.0, 200, glm_ease_quad_out);
	WAIT(800);
	INVOKE_TASK(easing_animate,&cam->pos[2], 4, 400, glm_ease_quad_inout);
	WAIT(1200);
	INVOKE_TASK(easing_animate,&cam->pos[2], 3, 400, glm_ease_quad_inout);
	WAIT(100);
	INVOKE_TASK(easing_animate,&cam->pos[2], 4, 400, glm_ease_quad_inout);
	WAIT(300);
	INVOKE_TASK(easing_animate,&cam->vel[1], 0.06, 200, glm_ease_quad_in);
}

void stage4_bg_init_fullstage() {
	Stage4DrawData *draw_data = stage4_get_draw_data();
	draw_data->ambient_color = *RGB(0.3, 0.3, 0.3);

	INVOKE_TASK(update_stage_3d);
	INVOKE_TASK(animate_bg_fullstage);
}

void stage4_bg_init_spellpractice(void) {
	stage_3d_context.cam.far = 300;

	stage_3d_context.cam.pos[2] = 4;
	stage_3d_context.cam.pos[1] = 200;
	stage_3d_context.cam.rot.v[0] = 80;

	stage_3d_context.cam.vel[1] = 0.05;
	stage4_bg_redden_corridor();
	INVOKE_TASK(update_stage_3d);
}

TASK(redden_corridor, NO_ARGS) {
	Stage4DrawData *draw_data = stage4_get_draw_data();
	int t = 100;

	for(int i = 0; i < t; i++) {
		color_approach(&draw_data->ambient_color, RGB(1, 0, 0), 1.0f / t);
		YIELD;
	}
}

void stage4_bg_redden_corridor(void) {
	INVOKE_TASK(redden_corridor);
}
