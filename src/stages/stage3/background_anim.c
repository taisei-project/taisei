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
#include "util/glm.h"

TASK(animate_bg) {
	Stage3DrawData *dd = stage3_get_draw_data();
	Camera3D *cam = &stage_3d_context.cam;

	dd->target_swing_strength = 0.2f;

	for(int t = 0;; ++t, YIELD) {
		float f = 1.0f / sqrtf(1.0f + t / 500.0f);
		glm_vec3_copy((vec3) { f, f, sqrtf(f) }, dd->ambient_color);

		float swing = sin(t / 100.0) * dd->swing_strength;
		cam->pos[0] = swing;
		cam->rot.yaw = swing * -8.0f;

		fapproach_asymptotic_p(&dd->swing_strength, dd->target_swing_strength, 0.005f, 1e-3f);

		vec4 f0 = { 0.60, 0.30, 0.60, 1.0 };
		vec4 f1 = { 0.20, 0.10, 0.30, 1.0 };
		glm_vec4_lerp(f0, f1, 1 - f, dd->fog_color);

		vec3 e0 = { 2.50, 0.80, 0.50 };
		vec3 e1 = { 0.20, 0.30, 0.50 };
		glm_vec3_lerp(e0, e1, 1 - f, dd->environment_color);
		glm_vec3_scale(dd->environment_color, 0.5 * f, dd->environment_color);

		stage3d_update(&stage_3d_context);

		if(global.boss && !boss_is_fleeing(global.boss)) {
			fapproach_asymptotic_p(&dd->boss_light_alpha, 1, 0.02f, 1e-3f);
		} else {
			fapproach_asymptotic_p(&dd->boss_light_alpha, 0, 0.02f, 1e-3f);
		}

		if(dd->boss_light_alpha > 0 && global.boss) {
			vec3 r;
			camera3d_unprojected_ray(cam, global.boss->pos, r);
			glm_vec3_scale(r, 14, r);
			glm_vec3_add(cam->pos, r, dd->boss_light.pos);
		}
	}
}

void stage3_bg_init_fullstage(void) {
	INVOKE_TASK(animate_bg);
}

void stage3_bg_init_spellpractice(void) {
	INVOKE_TASK(animate_bg);
}
