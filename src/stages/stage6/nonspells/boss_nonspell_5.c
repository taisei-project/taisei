/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

DEFINE_EXTERN_TASK(stage6_boss_nonspell_5) {
	STAGE_BOOKMARK(boss-non5);
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(stage6_boss_nonspell_baryons_common, ARGS.baryons);
	aniplayer_queue(&boss->ani, "snipsnip", 0);

	WAIT(100);
	int interval = difficulty_value(195, 190, 185, 180);

	for(;;) {
		play_sfx("shot_special1");

		int cnt = 5;
		for(int i = 0; i < cnt; ++i) {
			cmplx dir = cdir(M_PI / 4 * (1.0 / cnt * i - 0.5)) * cnormalize(global.plr.pos-boss->pos);

			for(int j = 0; j < 3; j++) {
				PROJECTILE(
					.proto = pp_bigball,
					.pos = boss->pos,
					.color = RGB(0,0.2,0.9),
					.move = move_asymptotic_simple(dir, 2 * j)
				);
			}
		}
		WAIT(interval);

		int w = difficulty_value(1, 1, 2, 2);
		cmplx dir = cnormalize(global.plr.pos - boss->pos);

		for(int x = -w; x <= w; x++) {
			for(int y = -w; y <= w; y++) {
				PROJECTILE(
					.proto = pp_bigball,
					.pos = boss->pos + 25 * (x + I * y) * dir,
					.color = RGB(0,0.2,0.9),
					.move = move_asymptotic_simple(dir, 3)
				);
			}
		}
		WAIT(interval);
	}
}
