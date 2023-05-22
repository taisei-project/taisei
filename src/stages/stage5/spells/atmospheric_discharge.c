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
	int shot_count = difficulty_value(7, 8, 9, 10);
	int shot_accel = difficulty_value(5, 6, 7, 8);
	int delay = 10;
	int waves = difficulty_value(7, 8, 9, 10);

	cmplx p1 = ARGS.p1;
	cmplx p2 = ARGS.p2;

	for(int n = 0; n < waves * 2; ++n) {
		for(real i = -shot_count * 0.5; i <= shot_count * 0.5; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = p1 + (p2 - p1) / shot_count * i,
				.color = RGBA(1 - 1 / (1 + fabs(0.1 * i)), 0.5 - 0.1 * fabs(i), 1, 0),
				.move = move_accelerated(0,
					(1 - 2 * (n & 1)) *
					(0.001 * shot_accel) *
					cnormalize(p2 - p1) * cdir(M_PI/2 + 0.2 * i)
				),
			);
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
	wander_bounds.bottom = VIEWPORT_H * 0.5;

	boss->move.attraction = 0.03;

	int mindelay = 30;
	int delay_step = 6;
	int delay = mindelay + 10 * delay_step;

	for(;;WAIT(delay)) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.4, wander_bounds);	INVOKE_SUBTASK_DELAYED(10, ball_shoot, boss->pos, boss->move.attraction_point);
		delay = imax(delay - delay_step, mindelay);
	}
}
