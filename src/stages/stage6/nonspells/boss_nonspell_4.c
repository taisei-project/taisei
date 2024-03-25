/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"


TASK(baryons_nonspell_movement, { BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	for(int t = 0;; t++) {
		for(int i = 0; i < NUM_BARYONS; i++) {
			baryons->target_poss[i] = baryons->center_pos + 150 * cdir(M_TAU/6 * i + 0.006 * t);
		}
		YIELD;
	}
}
	

DEFINE_EXTERN_TASK(stage6_boss_nonspell_baryons_common) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	INVOKE_SUBTASK(baryons_nonspell_movement, ARGS.baryons);

	int interval = difficulty_value(6, 5, 4, 3);
	real speed = difficulty_value(1.2, 1.4, 1.6, 1.8);

	WAIT(30);
	for(int t = 0;; t++) {
		play_sfx_loop("shot1_loop");
		for(int i = 0; i < NUM_BARYONS; i++) {
			real angle = (0.2 + 0.006 * interval) * t + i;

			real color_angle = angle + t * interval / 60.0;

			PROJECTILE(
				.proto = pp_ball,
				.pos = baryons->poss[i] + 40 * cdir(angle),
				.color = RGB(cos(color_angle), sin(color_angle), cos(color_angle + 2.1)),
				.move = move_asymptotic_simple(speed * cdir(angle), 3)
			);
		}

		WAIT(interval);
	}
}

DEFINE_EXTERN_TASK(stage6_boss_nonspell_4) {
	STAGE_BOOKMARK(boss-non4);
	stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(stage6_boss_nonspell_baryons_common, ARGS.baryons);
	STALL;
}
