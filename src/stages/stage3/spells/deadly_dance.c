/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../scuttle.h"

#include "common_tasks.h"
#include "global.h"

TASK(spawner_proj, { cmplx pos; MoveParams move; Color *color; int t; int i; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_wave,
		.pos = ARGS.pos,
		.color = ARGS.color,
		.move = ARGS.move,
	));

	int t = ARGS.t;
	int i = ARGS.i;

	WAIT(150);
	real a = (M_PI/(5 + global.diff) * i * 2);

	play_sfx("redirect");

	PROJECTILE(
		.proto = rng_chance(0.5) ? pp_thickrice : pp_rice,
		.pos = p->pos,
		.color = RGB(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3),
		.move = move_accelerated(0, 0.005 * cdir((M_TAU * sin(a / 5.0 + t / 20.0))))
	);
}

TASK(spawners, { BoxedBoss boss; real tfactor; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	int count = difficulty_value(15, 18, 21, 24);
	real tfactor = ARGS.tfactor;

	for(int time = 0;; time += WAIT(70)) {
		real angle_ofs = rng_angle();
		play_sfx("shot_special1");

		for(int i = 0; i < count; ++i) {
			real a = (M_PI/(5 + global.diff) * i * 2);
			INVOKE_TASK(spawner_proj,
				.pos = boss->pos,
				.color = RGB(0.3, 0.3 + 0.7 * psin((M_PI/(5 + global.diff) * i * 2) * 3 + time/50.0), 0.3),
				.move = move_accelerated(0, 0.02 * cdir(angle_ofs + a + time/10.0)),
				.t = time * tfactor,
				.i = i,
			);
		}
	}
}

TASK(balls, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	int count = difficulty_value(2, 4, 6, 8);

	for(int time = 0;; time += WAIT(35)) {
		real angle_ofs = rng_angle();
		play_sfx("shot1");

		for(int i = 0; i < count; ++i) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = boss->pos,
				.color = RGB(1.0, 1.0, 0.3),
				.move = move_asymptotic_simple(
					(0.5 + 3 * psin(time + M_PI / 3 * 2 * i)) * cdir(angle_ofs + time / 20.0 + M_PI / count * i * 2),
					1.5
				)
			);
		}
	}
}

TASK(limiters, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(int time = 0;; time += WAIT(3)) {
		play_sfx_loop("shot1_loop");

		for(int i = -1; i < 2; i += 2) {
			real c = psin(time/10.0);
			cmplx aim = cnormalize(global.plr.pos - boss->pos);
			cmplx v = 10 * (aim + (M_PI/4.0 * i * (1 - time / 2500.0)) * (1 - 0.5 * psin(time / 15.0)));
			PROJECTILE(
				.proto = pp_crystal,
				.pos = boss->pos,
				.color = RGBA_MUL_ALPHA(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7),
				.move = move_linear(v),
			);
		}
	}
}

DEFINE_EXTERN_TASK(stage3_spell_deadly_dance) {
	STAGE_BOOKMARK(dance);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.08);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move.attraction = 0;

	aniplayer_queue(&boss->ani, "dance", 0);
	real tfactor = difficulty_value(1.05, 1.5, 1.95, 2.4);

	INVOKE_SUBTASK(limiters, ENT_BOX(boss));
	INVOKE_SUBTASK(spawners, ENT_BOX(boss), .tfactor = tfactor);

	if(global.diff > D_Easy) {
		INVOKE_SUBTASK(balls, ENT_BOX(boss));
	}

	for(int time = 0;; ++time, YIELD) {
		real t = time * tfactor;
		real moverad = fmin(160, time/2.7);
		boss->pos = VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(t/50.0) * moverad * cdir(M_PI/2 * t/100.0);
	}
}
