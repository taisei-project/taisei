/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

TASK(forgotten_baryons_movement, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	for(int t = 0;; t++) {
		cmplx boss_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;
		baryons->center_pos = boss_pos;

		for(int i = 0; i < NUM_BARYONS; i++) {
			bool odd = i & 1;

			cmplx target = (1 - 2 * odd) * (300 + 100 * sin(t * 0.01)) * cdir(M_TAU * (i + 0.5 * odd) / 6 + 0.6 * sqrt(1 + t * t / 600.));

			baryons->target_poss[i] = boss_pos + target;
		}
		YIELD;
	}
}

TASK(forgotten_baryons_spawner, { BoxedEllyBaryons baryons;}) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	int interval = difficulty_value(76, 72, 68, 64);

	for(;;) {
		for(int i = 0; i < NUM_BARYONS; i++) {
			RNG_ARRAY(R, 2);
			cmplx pos = baryons->poss[i] + 60 * vrng_sreal(R[0]) + I * 60 * vrng_sreal(R[1]);

			if(cabs(pos - global.plr.pos) > 100) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = pos,
					.color = RGBA(1.0, 0.4, 1.0, 0.0),
					.move = move_linear(cnormalize(global.plr.pos - pos)),
				);
			}
		}

		WAIT(interval);
	}
}

TASK(forgotten_bullet, { cmplx pos; cmplx *diff; }) {
	real speed = difficulty_value(0.25, 0.25, 0.5, 0.5);

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_flea,
		.pos = ARGS.pos,
		.color = RGB(0.1 * rng_real(), 0.6, 1),
	));

	cmplx vel0 = speed * rng_dir();

	for(;;) {
		cmplx v = (*ARGS.diff)*0.00005;
		cmplx x = p->pos - global.plr.pos;

		real f1 = 1 + 2 * creal(v*conj(x)) + cabs2(x);
		real f2 = 1 - creal(v*v);
		real f3 = f1 - f2 * cabs2(x);
		p->pos = global.plr.pos + (f1 * v + f2 * x) / f3 + vel0 / (1 + 2 * cabs2(x) / VIEWPORT_W / VIEWPORT_W);

		YIELD;
	}
}

TASK(forgotten_orbiter, { BoxedProjectile parent; cmplx offset; }) {
	real angular_velocity = 0.03;
	Projectile *p = PROJECTILE(
		.proto = pp_plainball,
		.pos = NOT_NULL_OR_DIE(ENT_UNBOX(ARGS.parent))->pos + ARGS.offset,
		.color = RGBA(0.2, 0.4, 1.0, 0.0),
	);

	for(int t = 0;; t++) {
		Projectile *parent = ENT_UNBOX(ARGS.parent);
		if(parent == NULL) {
			p->move = move_linear(angular_velocity * I * ARGS.offset * cdir(t * angular_velocity));
			break;
		}

		p->pos = parent->pos + ARGS.offset * cdir(t * angular_velocity);
		YIELD;
	}
}

TASK(forgotten_orbiter_spawner, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	for(int i = 0;; i++) {
		play_sfx_ex("shot2", 10, false);

		Projectile *p = PROJECTILE(
			.proto = pp_bigball,
			.pos = boss->pos,
			.color = RGBA(0.5, 0.4, 1.0, 0.0),
			.move = move_linear(4 * I * cdir(-M_TAU / 5 * triangle(i/5./M_TAU)))
		);

		if(global.diff == D_Lunatic) {
			INVOKE_TASK(forgotten_orbiter, ENT_BOX(p), 40 * cdir(i / 20.0));
		}

		WAIT(20);
	}
}

TASK(forgotten_spawner) {
	cmplx diff = 0;
	cmplx plr_pos_old = global.plr.pos;

	int interval = difficulty_value(3, 3, 2, 2);

	for(int t = 0;; t++) {
		diff = global.plr.pos - plr_pos_old;
		plr_pos_old = global.plr.pos;

		play_sfx_loop("shot1_loop");

		if(t % interval == 0) {
			RNG_ARRAY(R, 2);
			cmplx pos = VIEWPORT_W * vrng_real(R[0]) + I * VIEWPORT_H * vrng_real(R[1]);

			if(cabs(pos - global.plr.pos) > 50) {
				INVOKE_SUBTASK(forgotten_bullet, pos, &diff);
			}
		}
		YIELD;
	}
}


DEFINE_EXTERN_TASK(stage6_spell_forgotten) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(forgotten_baryons_movement, ARGS.baryons, ENT_BOX(boss));
	if(global.diff > D_Easy) {
		INVOKE_SUBTASK(forgotten_baryons_spawner, ARGS.baryons);
	}

	INVOKE_SUBTASK_DELAYED(50, forgotten_spawner);
	if(global.diff > D_Normal) {
		INVOKE_SUBTASK_DELAYED(50, forgotten_orbiter_spawner, ENT_BOX(boss));
	}

	for(int t = 0;; t++) {
		boss->move = move_from_towards(
			boss->pos,
			VIEWPORT_W / 2.0 + 100 * I + VIEWPORT_W / 3.0 * round(sin(t)),
			0.04
		);
		WAIT(200);
	}
}
