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

TASK(spawn_square, {
	BoxedProjectileArray *projectiles;
	cmplx pos;
	cmplx dir;
	real width;
	int count;
	real speed;
}) {
	int delay = round(ARGS.width / (ARGS.count-1) / ARGS.speed);

	for(int i = 0; i < ARGS.count; i++) {
		for(int j = 0; j < ARGS.count; j++) {
			real x = ARGS.width * (-0.5 + j / (real) (ARGS.count-1));

			ENT_ARRAY_ADD(ARGS.projectiles, PROJECTILE(
				.proto = pp_ball,
				.pos = ARGS.pos + x * ARGS.dir * I,
				.color = RGB(0, 0.5, 1),
				.move = move_linear(ARGS.speed * ARGS.dir),
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
	real phase = 0.5;
	int steps = M_TAU/speed;

	for(int n = 2;; n++) {
		for(int i = 0; i < steps; i++) {
			real t = M_TAU/steps*i;

			Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));

			if(i % (steps/(2*n)) == 0) {
				spawn_apples(boss->pos);
			}

			scythe->pos = boss->pos + radius * sin(n*t) * cdir(t + phase) * I;
			YIELD;
		}
	}
}

static void scythe_touch_bullet(Projectile *p) {
	real acceleration = difficulty_value(0.01, 0.02, 0.03, 0.04);
	cmplx aim = cnormalize(p->move.velocity * I);
	real vis_scale = 0.75;
	real hit_scale = 0.75;

	p->color.r = 1.0;
	p->scale = vis_scale * (1 + I);
	p->collision_size *= hit_scale;

	auto p2 = PROJECTILE(
		.proto = p->proto,
		.pos = p->pos,
		.color = &p->color,
		.move = p->move,
	);

	p2->scale = vis_scale * (1 + I);
	p2->collision_size *= hit_scale;

	p->move.acceleration  += acceleration * aim;
	p2->move.acceleration -= acceleration * aim;

	play_sfx("redirect");
}

TASK(newton_scythe, { BoxedEllyScythe scythe; BoxedBoss boss; BoxedProjectileArray *projectiles; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	INVOKE_SUBTASK(newton_scythe_movement, ARGS.scythe, ARGS.boss);

	real effect_radius = difficulty_value(15, 20, 25, 30);
	real effect_radius_sq = effect_radius * effect_radius;

	for(;;) {
		ENT_ARRAY_FOREACH_COUNTER(ARGS.projectiles, int idx, Projectile *p, {
			if(cabs2(p->pos - scythe->pos) < effect_radius_sq) {
				scythe_touch_bullet(p);
				// Stop tracking this bullet
				ARGS.projectiles->array[idx] = (BoxedProjectile) { };
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

	INVOKE_SUBTASK_DELAYED(180, newton_scythe, ARGS.scythe, ENT_BOX(boss), &projectiles);

	real width = 100;
	int bullets_per_side = difficulty_value(4, 4, 5, 5);

	real square_speed = difficulty_value(2, 2.3, 2.6, 3);

	int n = 6;
	real o = 0;

	for(;;) {
		cmplx aim = cnormalize(global.plr.pos - boss->pos);

		for(int i = 0; i < n; ++i) {
			cmplx dir = aim * cdir((i + o) * M_TAU / n);
			cmplx pos = boss->pos + 100 * dir;
			INVOKE_SUBTASK(spawn_square, &projectiles, pos, -dir,
				.width = width,
				.count = bullets_per_side,
				.speed = square_speed,
			);
		}

		WAIT(60*(2/square_speed));
		o += 0.5;
	}
}


