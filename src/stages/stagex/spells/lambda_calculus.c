/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "coroutine/taskdsl.h"
#include "dynarray.h"
#include "entity.h"
#include "move.h"
#include "projectile.h"
#include "stages/stagex/yumemi.h"
#include "taisei.h"

#include "../stagex.h"
#include "global.h"
#include "util/glm.h"
#include "common_tasks.h"
#include "stage.h"

#define LAMBDA_MAX_NUM_PRODUCTS 8
#define LAMBDA_APPLICATION_RADIUS 5

typedef struct {
	MoveParams moves[LAMBDA_MAX_NUM_PRODUCTS];
	int types[LAMBDA_MAX_NUM_PRODUCTS];
	Color colors[LAMBDA_MAX_NUM_PRODUCTS];
	int count;

	BoxedProjectile ref;
} LambdaAbstraction;

typedef DYNAMIC_ARRAY(LambdaAbstraction) LambdaAbstractionArray;

static ProjPrototype *lambda_proto(int type) {
	ProjPrototype *protos[] = {
		pp_flea,
		pp_wave,
		pp_ball,
		pp_bullet,
	};
	assert(type >= 0 && type < ARRAY_SIZE(protos));
	return protos[type];
}

static Projectile *spawn_lambda_bullet(LambdaAbstraction *data, cmplx pos, MoveParams move) {
	play_sfx("shot1");
	Projectile *p = PROJECTILE(
		.proto = pp_flea,
		.pos = pos,
		.color = RGB(0.5,0.2,0.5),
		.move = move,
	);

	for(int i = 0; i < data->count; i++) {
		PROJECTILE(
			.proto = lambda_proto(data->types[i]),
			.pos = p->pos + 10 * cdir(M_TAU/data->count * i),
			.move = p->move,
			.color = &data->colors[i]
		);
	}
	return p;
}

static LambdaAbstraction *lambda_array_add(LambdaAbstractionArray *lambdas, LambdaAbstraction l) {
	dynarray_foreach_elem(lambdas, LambdaAbstraction *lold, {
		if(ENT_UNBOX(lold->ref) == NULL) {
			*lold = l;
			return lold;
		}
	});

	return dynarray_append(lambdas, l);
}	

static void lambda_apply(LambdaAbstractionArray *lambdas, LambdaAbstraction *l1, LambdaAbstraction *l2) {
	int lambda_population = 0;
	dynarray_foreach_elem(lambdas, auto l, {
		if(ENT_UNBOX(l->ref) != NULL) {
			lambda_population++;
		}
	});

	log_warn("%d", lambda_population);
	if(lambda_population > 50) {
		return;
	}
	
	auto p1 = NOT_NULL(ENT_UNBOX(l1->ref));
	auto p2 = NOT_NULL(ENT_UNBOX(l2->ref));
	play_sfx("redirect");
	
	
	for(int c = 0; c < l1->count; c++) {
		cmplx v = cdir(M_TAU * c / l1->count);

		if(l1->types[c] == 0) {
			auto lnew = lambda_array_add(lambdas, *l2);
			if(lnew != NULL) {
				auto proj = spawn_lambda_bullet(l2, p1->pos + 10 * v, l1->moves[c]);
				
				lnew->ref = ENT_BOX(proj);
			}
		} else {
			auto move = l1->moves[c];

			PROJECTILE(
				.proto = lambda_proto(l1->types[c]),
				.pos = p1->pos + 10 * v,
				.move = move,
				.color = &l1->colors[c],
			);
		}
	}

	kill_projectile(p1);
	kill_projectile(p2);
	l1->ref = (BoxedProjectile) {};
	l2->ref = (BoxedProjectile) {};
}

static void lambda_array_collisions(LambdaAbstractionArray *lambdas) {
	dynarray_foreach(lambdas, int i, LambdaAbstraction *l1, {
		auto p1 = ENT_UNBOX(l1->ref);
		if(p1 == NULL) {
			continue;
		}

		if(p1->birthtime >= global.frames-1) {
			continue;
		}
		
		dynarray_foreach(lambdas, int j, LambdaAbstraction *l2, {
			auto p2 = ENT_UNBOX(l2->ref);
			if(i <= j || ENT_UNBOX(l1->ref) == NULL || p2 == NULL) {
				continue;
			}

			if(p2->birthtime >= global.frames-1) {
				continue;
			}
			if(cabs(p1->pos-p2->pos) < LAMBDA_APPLICATION_RADIUS) {
				if(l1->count < l2->count) {
					lambda_apply(lambdas, l2, l1);
				} else {
					lambda_apply(lambdas, l1, l2);
				}
			}
		});
	});
}

TASK(lambda_calculus_cleanup, { LambdaAbstractionArray *lambdas; }) {
	dynarray_free_data(ARGS.lambdas);
}

TASK(lambda_array_process, { LambdaAbstractionArray *lambdas; }) {
	for(;;) {
		lambda_array_collisions(ARGS.lambdas);
		YIELD;
	}
}

static LambdaAbstraction random_abstraction(int count, uint64_t bits) {
	assert(count < LAMBDA_MAX_NUM_PRODUCTS);

	LambdaAbstraction ld = {};

	for(int i = 0; i < count; i++) {
		cmplx dir = cdir(M_TAU/count * i);
		ld.moves[i] = move_accelerated(dir, 0.02 * cdir(1.4) * dir);
		ld.colors[i] = *RGBA(0.8,0.2,0.0,0.4);
		ld.types[i] = (bits>>i)&3;
	}
	ld.count = count;
	return ld;
}

TASK(lambda_calculus_slave_move, { BoxedYumemiSlave slave; int type; int duration;}) {
	auto slave = TASK_BIND(ARGS.slave);

	int dir = 1 - ARGS.type * 2;
	for(int t = 0; t < ARGS.duration; t++) {
		slave->pos = global.boss->pos + 100 * dir * cdir(M_TAU / ARGS.duration * dir * t);
		YIELD;
	}
}

TASK(lambda_calculus_slave_track, { BoxedYumemiSlave slave; int type; int duration;}) {
	auto slave = TASK_BIND(ARGS.slave);
	int dir = 1 - ARGS.type * 2;
	for(int t = 0; t < ARGS.duration; t++) {
		slave->pos = global.boss->pos + 100 * dir;
		YIELD;
	}
}


TASK(lambda_calculus_slave, { cmplx pos; int type; LambdaAbstractionArray *lambdas; }) {
	auto slave = TASK_BIND(stagex_host_yumemi_slave(ARGS.pos, ARGS.type));

	int dir = 1-ARGS.type * 2;

	for(;;) {
		INVOKE_SUBTASK_DELAYED(50, common_charge, 0, RGBA(0.5, 0.2, 0.5, 0), 50, .sound = COMMON_CHARGE_SOUNDS, .anchor = &slave->pos);
		INVOKE_SUBTASK(lambda_calculus_slave_track, ENT_BOX(slave), ARGS.type, 100);
		WAIT(100);

		int nsteps = 9;
		int interval = 5;
		INVOKE_SUBTASK(lambda_calculus_slave_move, ENT_BOX(slave), ARGS.type, nsteps * interval);

		for(int i = -nsteps/2; i <= nsteps/2; i++) {
			cmplx offset = cdir(0.1*i*dir);

			cmplx target = offset*(global.plr.pos - slave->pos) + slave->pos;
			cmplx vel = 3*(target - slave->pos)/cabs(target - global.boss->pos);
			LambdaAbstraction ld = random_abstraction(7, global.frames*global.frames + ARGS.type*124);
			auto bullet = spawn_lambda_bullet(&ld, slave->pos, move_linear(vel));
			ld.ref = ENT_BOX(bullet);
			lambda_array_add(ARGS.lambdas, ld);

			WAIT(interval);
		}

	}
	
}

DEFINE_EXTERN_TASK(stagex_spell_lambda_calculus) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	LambdaAbstractionArray lambdas = {};
	dynarray_ensure_capacity(&lambdas, 100);
	INVOKE_SUBTASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, lambda_calculus_cleanup, &lambdas);

	INVOKE_SUBTASK(lambda_array_process, &lambdas);

	INVOKE_SUBTASK(lambda_calculus_slave, boss->pos + 100, 0, &lambdas);
	INVOKE_SUBTASK(lambda_calculus_slave, boss->pos - 100, 1, &lambdas);
	

	cmplx positions[] = {
		VIEWPORT_W/2.0 + 100*I,
		VIEWPORT_W - 100 +  500*I,
		100 + 500 * I,
	};

	for(int i = 0;; i++) {
		boss->move = move_towards(0, global.plr.pos, 0.01);
		WAIT(160);
	}
}
