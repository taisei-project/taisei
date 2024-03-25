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
#include "../kurumi.h"

#include "global.h"

typedef struct {
	cmplx pos;
	MoveParams move;
} Mover; // make this global?

TASK(mover_update, { Mover *m; }) {
	for(;;) {
		move_update(&ARGS.m->pos, &ARGS.m->move);
		YIELD;
	}
}


TASK(kurumi_aniwall_bullet, { cmplx pos; MoveParams move; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.color = RGB(1,0,0),
		.move = ARGS.move,
		.layer = LAYER_BULLET + rng_real() * 10
	));

	for(;;) {
		p->color.r = 0.3 + 0.7 * psin(4 * im(p->pos)/VIEWPORT_H + 0.03 * global.frames);
		YIELD;
	}
}

TASK(kurumi_aniwall_slave_move, { Mover *m; cmplx direction; }) {

	cmplx corners[] = {
		0,
		VIEWPORT_W,
		VIEWPORT_W + I * VIEWPORT_H,
		I * VIEWPORT_H,
	};

	real speed = 10;

	int order;
	int next;
	if(re(ARGS.direction) > 0) {
		next = 2;
		order = 1;
	} else {
		next = 3;
		order = -1;
	}

	for(;; next = (next + order + ARRAY_SIZE(corners)) % ARRAY_SIZE(corners)) {
		cmplx d = corners[next] - ARGS.m->pos;
		ARGS.m->move = move_linear(speed * cnormalize(d));

		int travel_time = cabs(d) / speed;
		WAIT(travel_time);
	}
}


TASK(kurumi_aniwall_slave, { cmplx pos; cmplx direction; }) {
	Mover m = {
		.pos = ARGS.pos,
		.move = move_accelerated(0, 0.2*ARGS.direction)
	};
	INVOKE_SUBTASK(mover_update, &m);
	INVOKE_SUBTASK(stage4_boss_slave_visual, &m.pos, .interval = 1);

	create_lasercurve2c(ARGS.pos, 50, 80, RGBA(1.0, 0.4, 0.4, 0.0), las_accel, 0, m.move.acceleration);

	real velocity_boost = re(ARGS.direction) > 0 ? 1 : 0.7;

	real target_edge = re(ARGS.direction) > 0 ? VIEWPORT_W : 0;
	int impact_time = sqrt(2 * fabs(re(ARGS.pos - target_edge) / re(m.move.acceleration)));

	WAIT(impact_time);

	m.move = move_linear(10 * I * sign(im(ARGS.direction)));

	INVOKE_SUBTASK(kurumi_aniwall_slave_move, &m, ARGS.direction);

	int next_gap = 0;

	int step = difficulty_value(4, 3, 2, 2);
	int max_gap = difficulty_value(4, 5, 6, 7);
	for(int i = 0;; i++, WAIT(step)) {
		cmplx vel = cdir(0.5*M_TAU/4 * sign(re(ARGS.direction))) * cnormalize(m.move.velocity) * velocity_boost;
		if(1) {
			if(i < next_gap) {
				play_sfx("shot1");
				INVOKE_TASK(kurumi_aniwall_bullet,
					.pos = m.pos,
					.move = move_linear(vel)
				);
			} else {
				next_gap += rng_i32_range(2, max_gap);
				if(global.diff > D_Normal) {
					PROJECTILE(
						.proto = pp_flea,
						.pos = m.pos,
						.color = RGBA(1,0,0,0.5),
						.move = move_linear(vel),
					);
				}
			}
		}
	}
}

DEFINE_EXTERN_TASK(kurumi_aniwall) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	b->move = move_from_towards(b->pos, (VIEWPORT_W + I * VIEWPORT_H) * 0.5, 0.0005);
	b->move.retention = cdir(0.01);

	aniplayer_queue(&b->ani, "muda", 0);
	play_sfx("laser1");
	INVOKE_SUBTASK(kurumi_aniwall_slave, .pos = b->pos, .direction = cdir(0.4));
	INVOKE_SUBTASK(kurumi_aniwall_slave, .pos = b->pos, .direction = cdir(M_PI - 0.4));
	STALL;
}
