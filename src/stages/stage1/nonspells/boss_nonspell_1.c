/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

DEFINE_EXTERN_TASK(stage1_boss_nonspell_1) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(;;) {
		WAIT(20);
		aniplayer_queue(&boss->ani, "(9)", 3);
		aniplayer_queue(&boss->ani, "main", 0);
		play_sfx("shot_special1");

		const int num_shots = 5;
		const int num_projs = difficulty_value(9, 10, 11, 12);

		for(int shot = 0; shot < num_shots; ++shot) {
			for(int i = 0; i < num_projs; ++i) {
				cmplx shot_org = boss->pos;
				cmplx aim = cdir(i*M_TAU/num_projs + carg(global.plr.pos - shot_org));
				real speed = 3 + shot / 3.0;
				real boost = shot * 0.7;

				PROJECTILE(
					.proto = pp_plainball,
					.pos = shot_org,
					.color = RGB(0, 0, 0.5),
					.move = move_asymptotic_simple(speed * aim, boost),
				);
			}

			WAIT(2);
		}

		WAIT(10);

		for(int t = 0, i = 0; t < 60; ++i) {
			play_sfx_loop("shot1_loop");
			real speed0 = difficulty_value(4.0, 5.0, 6.0, 6.0);
			real speed1 = difficulty_value(3.0, 4.0, 6.0, 8.0);
			real angle = rng_sreal() * M_PI/8.0;
			real sign =  1 - 2 * (i & 1);
			cmplx shot_org = boss->pos - 42*I + 30 * sign;
			PROJECTILE(
				.proto = pp_crystal,
				.pos = shot_org,
				.color = RGB(0.3, 0.3, 0.8),
				.move = move_asymptotic_halflife(speed0 * -I * cdir(angle), speed1 * I, 30.0),
				.max_viewport_dist = 256,
			);
			t += WAIT(difficulty_value(3, 3, 1, 1));
		}

		boss->move.attraction = 0.02;
		WAIT(20);
		stage1_cirno_wander(boss, 60, 200);
		WAIT(30);

		for(int t = 0, i = 0; t < 150; ++i) {
			play_sfx("shot1");
			float dif = rng_angle();
			for(int shot = 0; shot < 20; ++shot) {
				cmplx aim = cdir(M_TAU/8 * shot + dif);
				real speed = 3.0 + i / 4.0;

				PROJECTILE(
					.proto = pp_plainball,
					.pos = boss->pos,
					.color = RGB(0.04 * i, 0.04 * i, 0.4 + 0.04 * i),
					.move = move_asymptotic_simple(speed * aim, 2.5),
				);
			}

			t += WAIT(difficulty_value(25, 25, 15, 10));
		}

		boss->move.attraction = 0.04;
		stage1_cirno_wander(boss, 180, 200);
	}
}
