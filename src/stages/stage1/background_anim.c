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

void stage1_bg_raise_camera(void) {
	Stage1DrawData *draw_data = stage1_get_draw_data();
	draw_data->pitch_target = 75;
}

void stage1_bg_enable_snow(void) {
	Stage1DrawData *draw_data = stage1_get_draw_data();
	draw_data->snow.opacity_target = 1;
}

void stage1_bg_disable_snow(void) {
	Stage1DrawData *draw_data = stage1_get_draw_data();
	draw_data->snow.opacity_target = 0;
}

TASK(animate_bg, { Stage1DrawData *draw_data; }) {
	Stage1DrawData *draw_data = ARGS.draw_data;
	YIELD;

	for(int t = 0;; t += WAIT(1)) {
		stage3d_update(&stage_3d_context);

		stage_3d_context.crot[1] = 2.0f * sinf(t/113.0f);
		stage_3d_context.crot[2] = 1.0f * sinf(t/132.0f);

		fapproach_asymptotic_p(&stage_3d_context.crot[0], draw_data->pitch_target, 0.01, 1e-5);
		fapproach_asymptotic_p(&draw_data->fog.near, draw_data->fog.near_target, 0.001, 1e-5);
		fapproach_asymptotic_p(&draw_data->fog.far, draw_data->fog.far_target, 0.001, 1e-5);
		fapproach_p(&draw_data->snow.opacity, draw_data->snow.opacity_target, 1.0 / 180.0);
	}
}

void stage1_bg_init_fullstage(void) {
	Stage1DrawData *draw_data = stage1_get_draw_data();

	draw_data->fog.near_target = 0.5;
	draw_data->fog.far_target = 1.5;
	draw_data->snow.opacity_target = 0.0;
	draw_data->pitch_target = 60;

	draw_data->fog.near = 0.2;
	draw_data->fog.far = 1.0;
	draw_data->snow.opacity = draw_data->snow.opacity_target;

	stage_3d_context.crot[0] = draw_data->pitch_target;
	stage_3d_context.cx[2] = 700;
	stage_3d_context.cv[1] = 8;

	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
	stage_3d_context.cam.near = 500;
	stage_3d_context.cam.far = 10000;

	// WARNING: Don't touch this unless know exactly what you're doing.
	// The water positioning black magic doesn't look right without this stretching.
	stage_3d_context.cam.aspect = (0.75f * VIEWPORT_W) / VIEWPORT_H;

	INVOKE_TASK(animate_bg, draw_data);
}

void stage1_bg_init_spellpractice(void) {
	stage1_bg_init_fullstage();
	stage1_bg_raise_camera();
	stage1_bg_enable_snow();

	Stage1DrawData *draw_data = stage1_get_draw_data();
	draw_data->fog.near = draw_data->fog.near_target;
	draw_data->fog.far = draw_data->fog.far_target;
	draw_data->snow.opacity = draw_data->snow.opacity_target;
	stage_3d_context.crot[0] = draw_data->pitch_target;
}
