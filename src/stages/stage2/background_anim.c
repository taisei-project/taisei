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
#include "coroutine.h"
#include "common_tasks.h"
#include "util/glm.h"

TASK(animate_hina_mode) {
	Stage2DrawData *draw_data = stage2_get_draw_data();

	int dur = 10000;
	Color fog_color_start = draw_data->fog.color;
	Color fog_color_final = *RGBA(0.1,0.1,1.0,1.0);

	for(int i = 0; i < dur; i++) {
		YIELD;
		draw_data->hina_lights = glm_ease_linear(i/(real)dur);
		draw_data->fog.color = *color_lerp(&fog_color_start, &fog_color_final, draw_data->hina_lights);
	}
}

void stage2_bg_engage_hina_mode(void) {
	INVOKE_TASK(animate_hina_mode);
}


TASK(animate_bg_fullstage) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = 8;
	cam->pos[0] = 10;
	cam->rot.v[0] = 50;
	cam->rot.v[2] = -90;

	Stage2DrawData *draw_data = stage2_get_draw_data();
	draw_data->fog.color = *RGBA(0.5, 0.6, 0.9, 1.0);

	real dur = 600;
	glm_ease_t func = glm_ease_quad_inout;
	INVOKE_TASK(common_easing_animate,&cam->rot.v[0], 40, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate,&cam->rot.v[2], -2.5f, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate,&cam->pos[0], 0, dur, func);
	INVOKE_TASK(common_easing_animate,&cam->vel[1], 0.05f, dur, func);

	INVOKE_TASK(common_easing_animate, &cam->rot.v[0], 40, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate, &cam->rot.v[2], -2.5f, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate, &cam->pos[0], 0, dur, func);
	INVOKE_TASK(common_easing_animate, &cam->vel[1], 0.05f, dur, func);

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_fullstage(void) {
	INVOKE_TASK(animate_bg_fullstage);
}

TASK(animate_bg_spellpractice) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = 8;
	cam->pos[0] = 0;
	cam->rot.v[0] = 40;
	cam->rot.v[2] = -2.5f;
	cam->vel[1] = 0.05f;

	Stage2DrawData *draw_data = stage2_get_draw_data();
	draw_data->fog.color = *RGBA(0.1, 0.1, 1, 1.0);
	draw_data->hina_lights = 0.0;

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg_spellpractice);
}
