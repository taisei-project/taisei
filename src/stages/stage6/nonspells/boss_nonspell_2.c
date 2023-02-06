/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

DEFINE_EXTERN_TASK(stage6_boss_nonspell_2) {
	STAGE_BOOKMARK(boss-non2);

	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);
	aniplayer_queue(&boss->ani, "snipsnip", 0);

	INVOKE_SUBTASK(stage6_boss_nonspell_scythe_common, ARGS.scythe);

	int interval = difficulty_value(3, 3, 2, 2);
	
	for(int t = 1;; t++) {
		cmplx dir = sin(t * 0.35) * cdir(t*0.02);

		PROJECTILE(
			.proto = pp_plainball,
			.pos = boss->pos + 50 * dir,
			.color = RGB(0,0,0.7),
			.move = move_asymptotic_simple(2*cnormalize(dir), 3)
		);

		WAIT(interval);
	}
}
