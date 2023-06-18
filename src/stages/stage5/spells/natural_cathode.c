/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

static cmplx cathode_laser(Laser *l, float t) {
	return l->pos + l->args[0] * sqrt(200 * t) * cdir(creal(l->args[1]) * t);
}

TASK(balls, { BoxedBoss boss; }) {
	auto boss = TASK_BIND(ARGS.boss);

	real speedmod = difficulty_value(1, 1, 1, 0.7);
	int bursts = difficulty_value(3, 4, 5, 6);
	int c = 10;

	cmplx r = cdir(M_TAU / c);
	cmplx dir = -I;

	for(int x = 0; x < bursts; ++x) {
		play_sfx("shot_special1");

		for(int i = 0; i < c; ++i) {
			Projectile *p = PROJECTILE(
				.proto = pp_bigball,
				.pos = boss->pos - 40 * dir * rng_sreal(),
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
				.max_viewport_dist = 128,
			);

			real s = (1 - 2 * (x & 1));

			INVOKE_TASK(iku_induction_bullet, {
				.p = ENT_BOX(p),
				.radial_vel = speedmod * dir * s,
				.angular_vel = speedmod * 0.01 * -I,
				.inverted = false,
			});

			dir *= r;
			WAIT(1);
		}

		WAIT(10);
	}
}

DEFINE_EXTERN_TASK(stage5_spell_natural_cathode) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	int c = difficulty_value(7, 8, 9, 9);
	cmplx r = cdir(M_TAU / c);
	cmplx r2 = cdir(3 * M_PI / 2);

	Rect bounds = viewport_bounds(120);
	bounds.top = 100;
	bounds.bottom = VIEWPORT_H * 0.4;

	for(;;) {
		cmplx dir = rng_dir();
		real s = rng_sign();

		for(int x = 0; x < 3; x++, WAIT(difficulty_value(55, 50, 45, 40))) {
			play_sfx("laser1");

			aniplayer_hard_switch(&boss->ani, (x & 1) ? "dashdown_left" : "dashdown_right", 1);
			aniplayer_queue(&boss->ani, "main", 0);

			for(int i = 0; i < c; i++) {
				create_lasercurve2c(boss->pos, 60, 260, RGBA(0.4, 1, 1, 0), cathode_laser,
					2 * dir,
					0.015 * s
				);

				dir *= r;
			}

			dir *= r2;
			s = -s;
		}

		boss->move.attraction = 0.03;
		boss->move.attraction_point = common_wander(
			boss->pos,
			VIEWPORT_W * rng_range(0.1, 0.2),
			bounds
		);

		WAIT(60);
		boss->move.attraction = 0.00;
		INVOKE_SUBTASK(balls, ENT_BOX(boss));
	}
}
