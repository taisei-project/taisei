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

TASK(spawn_square, { BoxedProjectileArray *projectiles; cmplx pos; cmplx dir; real width; int count; real speed; }) {

	int delay = round(ARGS.width / (ARGS.count-1) / ARGS.speed);
	for(int i = 0; i < ARGS.count; i++) {
		for(int j = 0; j < ARGS.count; j++) {
			real x = ARGS.width * (-0.5 + j / (real) (ARGS.count-1));

			ENT_ARRAY_ADD(ARGS.projectiles, PROJECTILE(
				.proto = pp_ball,
				.pos = ARGS.pos + x * ARGS.dir * I,
				.color = RGB(0, 0.5, 1),
				.move = move_accelerated(ARGS.speed * ARGS.dir, 0*0.01*I),
			));
		}
		play_sfx("shot2");
		WAIT(delay);
	}

}

static void spawn_apples(cmplx pos) {
	int apple_count = difficulty_value(0, 5, 7, 10);
	real offset_angle = rng_angle();

	for(int i = 0; i < apple_count; i++) {
		cmplx dir = cdir(M_TAU / apple_count * i + offset_angle);
		PROJECTILE(
			.pos = pos,
			.proto = pp_apple,
			.move = move_accelerated(dir, 0.03*dir),
			.color = RGB(1.0, 0.5, 0),
			.layer = LAYER_BULLET | 0xffff, // force them to render on top of the other bullets
		);
	}
}

TASK(newton_scythe_movement, { BoxedEllyScythe scythe; BoxedBoss boss; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	scythe->spin = 0.5;

	real radius = 300;
	real speed = 0.01;
	int steps = M_TAU/speed;

	for(int n = 2;; n++) {
		for(int i = 0; i < steps; i++) {
			real t = M_TAU/steps*i;

			Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));

			if(i % (steps/(2*n)) == 0) {
				spawn_apples(boss->pos);
			}

			scythe->pos = boss->pos + radius * sin(n*t) * cdir(t) * I;
			YIELD;
		}
	}
}

TASK(newton_scythed_proj, { BoxedProjectile proj; }) {
	Projectile *p = TASK_BIND(ARGS.proj);
	cmplx aim = cnormalize(global.plr.pos - p->pos);
	play_sfx("redirect");

	real acceleration = difficulty_value(0.015, 0.02, 0.03, 0.04);

	p->move.velocity = 0;
	p->move.acceleration = acceleration * aim;
}

TASK(newton_scythe, { BoxedEllyScythe scythe; BoxedBoss boss; BoxedProjectileArray *projectiles; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	INVOKE_SUBTASK(newton_scythe_movement, ARGS.scythe, ARGS.boss);

	real effect_radius = 30;

	for(;;) {
		ENT_ARRAY_FOREACH(ARGS.projectiles, Projectile *p, {
			real distance = cabs(p->pos - scythe->pos);
			if(distance < effect_radius && p->color.r != 1.0) {
				p->color.r = 1.0;
				INVOKE_TASK(newton_scythed_proj, ENT_BOX(p));
			}
		});

		ENT_ARRAY_COMPACT(ARGS.projectiles);

		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_spell_newton) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	int proj_count = 1024;
	DECLARE_ENT_ARRAY(Projectile, projectiles, proj_count);

	INVOKE_SUBTASK(newton_scythe, ARGS.scythe, ENT_BOX(boss), &projectiles);

	real width = 100;
	int bullets_per_side = 5;

	real square_speed = difficulty_value(2, 2.3, 2.6, 3);

	for(int i = 0;; i++) {
		for(int s = -1; s <= 1; s += 2) {
			cmplx dir = cdir((5 * M_PI / 6 + 0.1) * i) * I;
			cmplx pos = boss->pos + 100 * dir;
			INVOKE_SUBTASK(spawn_square, &projectiles, pos, -s*dir,
				       .width = width,
				       .count = bullets_per_side,
				       .speed = square_speed
			);
		}

		WAIT(20*(2/square_speed));
	}
}


