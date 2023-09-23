/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

TASK(ball_shoot, { cmplx p1; cmplx p2; }) {
	int shot_count = difficulty_value(5, 6, 8, 10);
	int shot_accel = difficulty_value(5, 6, 7, 8);
	int delay = 10;
	int waves = difficulty_value(7, 8, 9, 10);

	cmplx p1 = ARGS.p1;
	cmplx p2 = ARGS.p2;

	cmplx dir = cnormalize(p2 - p1);

	for(int n = 0; n < waves * 2; ++n) {
		real s = (1.0 - 2.0 * (n & 1));

		for(real i = -shot_count * 0.5; i <= shot_count * 0.5; i++) {
			auto p = PROJECTILE(
				.proto = pp_ball,
				.pos = p1 + (p2 - p1) / shot_count * i,
				.color = RGBA(1 - 1 / (1 + fabs(0.1 * i)), 0.5 - 0.1 * fabs(i), 1, 0),
				.move = move_accelerated(
					s * dir * cdir(M_PI - 0.5 * i * s),
					s * (0.001 * shot_accel) * dir * cdir(M_PI/2 + 0.2 * i * s)
				),
				.max_viewport_dist = 100,
			);

			if(s < 0) {
				SWAP(p->color.r, p->color.b);
			}
		}

		play_sfx("shot_special1");
		play_sfx("redirect");

		WAIT(delay);
	}
}

DEFINE_EXTERN_TASK(stage5_spell_atmospheric_discharge) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect wander_bounds = viewport_bounds(32);
	wander_bounds.top = 60;
	wander_bounds.bottom = VIEWPORT_H * 0.45;

	boss->move.attraction = 0.03;

	int mindelay = 40;
	int delay_step = 8;
	int delay = mindelay + 10 * delay_step;

	for(;;WAIT(delay)) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.4, wander_bounds);	INVOKE_SUBTASK_DELAYED(10, ball_shoot, boss->pos, boss->move.attraction_point);
		delay = max(delay - delay_step, mindelay);
	}
}
