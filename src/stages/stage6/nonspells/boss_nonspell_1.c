/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../elly.h"

DEFINE_EXTERN_TASK(stage6_boss_nonspell_scythe_common) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	cmplx center = ELLY_DEFAULT_POS;

	scythe->spin = 0.2;
	scythe->move = move_from_towards(scythe->pos, center, 0.1);
	WAIT(40);
	scythe->move.retention = 0.9;

	for(int t = 0;; t++) {

		real theta = 0.01 * t * (1 + 0.001 * t) + M_PI/2;
		scythe->pos = center + 200 * cos(theta) * (1 + sin(theta) * I) / (1 + pow(sin(theta), 2));

		cmplx dir = cdir(scythe->angle);
		real vel = difficulty_value(1.4, 1.8, 2.2, 2.6);

		cmplx color_dir = cdir(3 * theta);

		if(t > 50) {
			play_sfx_loop("shot1_loop");
			PROJECTILE(
				.proto = pp_ball,
				.pos = scythe->pos + 60 * dir * cdir(1.2),
				.color = RGB(creal(color_dir), cimag(color_dir), creal(color_dir * cdir(2.1))),
				.move = move_accelerated(0, 0.01 *vel * dir),
			);
		}

		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_boss_nonspell_1) {
	STAGE_BOOKMARK(boss-non1);

	stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(stage6_boss_nonspell_scythe_common, ARGS.scythe);

	STALL;
}
