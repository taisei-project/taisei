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

void stage5_update(void) {
	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
	stage_3d_context.cam.near = 100;
	stage_3d_context.cam.far = 20000;

	Stage5DrawData *stage5_draw_data = stage5_get_draw_data();

	TIMER(&global.timer);
	float w = 0.005;

	stage5_draw_data->stairs.rotshift += stage5_draw_data->stairs.omega;
	stage_3d_context.crot[0] += stage5_draw_data->stairs.omega*0.5;
	stage5_draw_data->stairs.rad += stage5_draw_data->stairs.omega*20;

	int rot_time = 6350;

	FROM_TO(rot_time, rot_time+50, 1) {
		stage5_draw_data->stairs.omega -= 0.005;
	}

	FROM_TO(rot_time+200, rot_time+250, 1) {
		stage5_draw_data->stairs.omega += 0.005;
	}

	stage_3d_context.cx[0] = stage5_draw_data->stairs.rad*cos(-w*global.frames);
	stage_3d_context.cx[1] = stage5_draw_data->stairs.rad*sin(-w*global.frames);
	stage_3d_context.cx[2] = -1700+w*3000/M_PI*global.frames;

	stage_3d_context.crot[2] = stage5_draw_data->stairs.rotshift-180/M_PI*w*global.frames;

	stage5_draw_data->stairs.light_strength *= 0.98;

	if (frand() < 0.01) {
		stage5_draw_data->stairs.light_strength = 5+5*frand();
	}

	stage3d_update(&stage_3d_context);
}
