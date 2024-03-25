/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "global.h"
#include "common_tasks.h"

TASK(change_velocity, { BoxedProjectile p; cmplx v; }) {
	Projectile *p = TASK_BIND(ARGS.p);
	p->move = move_asymptotic_halflife(p->move.velocity, ARGS.v, 20);
	spawn_projectile_highlight_effect(p);
	play_sfx("shot3");
}

TASK(wheel, { BoxedBoss boss; real spin_dir; int duration; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real spin_dir = ARGS.spin_dir;
	int duration = ARGS.duration;

	int period = difficulty_value(4, 3, 3, 3);
	real d = difficulty_value(0, 0, 1, 2);
	int arms = 6 + d;
	int slowdown_threshold = 300;
	int t_ofs = 60;

	INVOKE_SUBTASK(common_charge, boss->pos, RGBA(0.3, 0.2, 1.0, 0), t_ofs, .sound = COMMON_CHARGE_SOUNDS);
	WAIT(t_ofs);

	for(int t = t_ofs; t < t_ofs + duration; t += WAIT(period)) {
		play_sfx_loop("shot1_loop");
		real speed;

		if(t > slowdown_threshold) {
			speed = 2 + 8 * exp(-(t - slowdown_threshold) / 70.0);
		} else {
			// speed = 6 + 4 * fmin(0, t / 30.0);
			speed = 10;
		}

		int max_delay = 60;
		int min_delay = max_delay - clamp((t - 100)/20, 0, 50) - 1;

		for(int i = 1; i < arms; i++) {
			real a = spin_dir * M_TAU / (arms - 1) * (i + (1 + 0.4 * d) * pow(t/200.0, 2));
			cmplx dir = cdir(a);

			Projectile *p = PROJECTILE(
				.proto = pp_crystal,
				.pos = boss->pos,
				.color = RGB(log1p(t * 1e-3), 0, 0.2),
				.move = move_asymptotic_halflife(15 * dir, 0, 5),
			);

			INVOKE_TASK_DELAYED(rng_irange(min_delay, max_delay), change_velocity, ENT_BOX(p), speed * dir);
		}
	}
}

DEFINE_EXTERN_TASK(stage2_spell_wheel_of_fortune) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + VIEWPORT_H/2*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	aniplayer_queue(&boss->ani, "guruguru", 0);

	real spin_dir = rng_sign();
	int duration = 600;
	int overlap = difficulty_value(-60, 40, 60, 60);

	for(;;WAIT(duration - overlap)) {
		INVOKE_SUBTASK(wheel, ENT_BOX(boss), spin_dir, duration);
		spin_dir *= -1;
	}
}
