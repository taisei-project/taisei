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
#include "common_tasks.h"

TASK(update_stage_3d) {
	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

TASK(animate_bg_fullstage) {
	Camera3D *cam = &stage_3d_context.cam;

	cam->pos[2] = -1.8;
	cam->pos[1] = -30;
	cam->rot.v[0] = 80;
	cam->vel[1] = 0.017;
	cam->vel[2] = 0.008;

	WAIT(400);
	INVOKE_TASK(common_easing_animate,&cam->vel[1], 0.013, 200, glm_ease_sine_inout);
	INVOKE_TASK(common_easing_animate,&cam->vel[2], 0.0, 200, glm_ease_quad_out);
	WAIT(800);
	INVOKE_TASK(common_easing_animate,&cam->pos[2], 4, 400, glm_ease_quad_inout);
	WAIT(1200);
	INVOKE_TASK(common_easing_animate,&cam->pos[2], 3, 400, glm_ease_quad_inout);
	WAIT(100);
	INVOKE_TASK(common_easing_animate,&cam->pos[2], 4, 400, glm_ease_quad_inout);
	WAIT(300);
	INVOKE_TASK(common_easing_animate,&cam->vel[1], 0.06, 200, glm_ease_quad_in);
}

static void stage4_bg_init_common(void) {
	Stage4DrawData *draw_data = stage4_get_draw_data();
	draw_data->ambient_color = *RGB(0.3, 0.3, 0.3);
	stage_3d_context.cam.far = 300;

	glm_vec3_copy((vec3) { 0.8f, 0.4f, 0.2f }, draw_data->corridor.torch_light.base);
	draw_data->corridor.torch_light.r_factor = 0.1;
	draw_data->corridor.torch_light.g_factor = 0.05;

	// nstate goes from 1 to 0 linearly as the particle evolves
	// particle color = color_base + (nstate * color_nstate) + (nstate^2 * color_nstate2)
	glm_vec3_copy((vec3) { 10.0f, 0.0f, 0.0f }, draw_data->corridor.torch_particles.c_base);
	glm_vec3_copy((vec3) {  0.0f, 0.0f, 0.8f }, draw_data->corridor.torch_particles.c_nstate);
	glm_vec3_copy((vec3) {  0.0f, 3.2f, 0.0f }, draw_data->corridor.torch_particles.c_nstate2);
}

void stage4_bg_init_fullstage(void) {
	stage4_bg_init_common();
	INVOKE_TASK(update_stage_3d);
	INVOKE_TASK(animate_bg_fullstage);
}

void stage4_bg_init_spellpractice(void) {
	stage4_bg_init_common();

	stage_3d_context.cam.pos[2] = 4;
	stage_3d_context.cam.pos[1] = 200;
	stage_3d_context.cam.rot.v[0] = 80;
	stage_3d_context.cam.vel[1] = 0.06;

	INVOKE_TASK(update_stage_3d);
// 	INVOKE_TASK_DELAYED(60, common_call_func, stage4_bg_redden_corridor);
}

void stage4_bg_redden_corridor(void) {
	// TODO: maybe a nicer transition effect based on proximity of the lights

	Stage4DrawData *draw_data = stage4_get_draw_data();
	INVOKE_TASK(common_easing_animate_vec3,
		&draw_data->corridor.torch_light.base,
		{ 0.8f, 0.1f, 0.2f },
		60,
		glm_ease_quad_out
	);
	INVOKE_TASK(common_easing_animate_vec3,
		&draw_data->corridor.torch_particles.c_base,
		{ 10.0f, 0.0, 0.0f },
		60,
		glm_ease_quad_out
	);
	INVOKE_TASK(common_easing_animate_vec3,
		&draw_data->corridor.torch_particles.c_nstate,
		{ 10.0f, 0.2f, 0.3f },
		80,
		glm_ease_quad_out
	);
	INVOKE_TASK(common_easing_animate_vec3,
		&draw_data->corridor.torch_particles.c_nstate2,
		{ 10.0f, 0.2f, 0.2f },
		56,
		glm_ease_quad_out
	);
}
