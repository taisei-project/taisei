/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "background_anim.h"
#include "draw.h"

#include "stageutils.h"
#include "coroutine.h"
#include "common_tasks.h"
#include "util/glm.h"

TASK(animate_color_shift) {
	Stage2DrawData *draw_data = stage2_get_draw_data();

	int dur = 12000;
	Color fog_color_start = draw_data->fog.color;
	Color fog_color_final = *RGBA(0.1, 0.1, 1.0, 1.0);

	for(int i = 0; i < dur; i++, YIELD) {
		draw_data->fog.color = *color_lerp(
			&fog_color_start,
			&fog_color_final,
			i/(float)dur
		);
	}
}

void stage2_bg_begin_color_shift(void) {
	INVOKE_TASK(animate_color_shift);
}

void stage2_bg_enable_hina_lights(void) {
	Stage2DrawData *draw_data = stage2_get_draw_data();
	INVOKE_TASK(common_easing_animate,
		.value = &draw_data->hina_lights,
		.to = 1.0f,
		.duration = 60,
		.ease = glm_ease_sine_in,
	);
}

#include "camcontrol.h"

TASK(animate_bg_fullstage) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = 8;
	cam->pos[0] = 10;
	cam->rot.v[0] = 70;
	cam->rot.v[2] = -90;

	Stage2DrawData *draw_data = stage2_get_draw_data();
	draw_data->fog.color = *RGBA(0.45, 0.5, 0.5, 1);
	draw_data->fog.end = 1.0;
	INVOKE_TASK(common_easing_animate_vec4,
		&draw_data->fog.color.rgba, { 0.04, 0.06, 0.09, 1 }, 1600, glm_ease_quad_out);

	INVOKE_TASK_DELAYED(155, common_easing_animate,
		&draw_data->fog.end, 3.8, 2000, glm_ease_quad_out);

	real dur = 600;
	glm_ease_t func = glm_ease_quad_inout;
	INVOKE_TASK(common_easing_animate, &cam->rot.v[0], 50, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate, &cam->rot.v[2], -6, dur, glm_ease_quad_out);
	INVOKE_TASK(common_easing_animate, &cam->pos[0], -0.38, dur, func);
	INVOKE_TASK(common_easing_animate, &cam->pos[2], 8, dur, func);
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
	cam->pos[0] = -0.38;
	cam->rot.v[0] = 50;
	cam->rot.v[2] = -6;
	cam->vel[1] = 0.05f;

	Stage2DrawData *draw_data = stage2_get_draw_data();
	draw_data->fog.color = *RGBA(0.1, 0.1, 1, 1.0);
	draw_data->hina_lights = 1.0;
	draw_data->fog.end = 3.8;

	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

void stage2_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg_spellpractice);
}
