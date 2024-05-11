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
	dynarray_foreach(lambdas, int i, LambdaAbstraction *lold, {
	    i=i;
		if(ENT_UNBOX(lold->ref) == NULL) {
			*lold = l;
			return lold;
		}
	});
	if(lambdas->num_elements < 100) {
    	return dynarray_append(lambdas, l);
    }
    return NULL;
}	

static void lambda_apply(LambdaAbstractionArray *lambdas, LambdaAbstraction *l1, LambdaAbstraction *l2) {
	auto p1 = NOT_NULL(ENT_UNBOX(l1->ref));
	auto p2 = NOT_NULL(ENT_UNBOX(l2->ref));
	play_sfx("redirect");
	
	// to prevent chain reactions
	cmplx offset = rng_dir();
	
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
			move.velocity *= offset;
			move.acceleration *= offset;

			PROJECTILE(
				.proto = lambda_proto(l1->types[c]),
				.pos = p1->pos + 10 * offset * v,
				.move = move,
				.color = &l1->colors[c],
			);
		}
	}

	kill_projectile(p1);
	kill_projectile(p2);
	memset(&l1->ref, 0, sizeof(l1->ref));
	memset(&l2->ref, 0, sizeof(l2->ref));
}

static void lambda_array_collisions(LambdaAbstractionArray *lambdas) {
	dynarray_foreach(lambdas, int i, LambdaAbstraction *l1, {
	    auto p1 = ENT_UNBOX(l1->ref);
	    if(p1 == NULL) {
	    	continue;
	    }
		// XXX: how to do without internal flag?
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

	LambdaAbstraction ld;
	memset(&ld, 0, sizeof(ld));
	
	for(int i = 0; i < count; i++) {
		cmplx dir = cdir(M_TAU/count * i);
		ld.moves[i] = move_accelerated(dir, 0.02 * cdir(1.4) * dir);
		ld.colors[i] = *RGBA(0.8,0.2,0.0,0.4);
		ld.types[i] = (bits>>i)&3;
	}
	ld.count = count;
	return ld;
}

TASK(lambda_calculus_slave_move, { BoxedYumemiSlave slave; int type;}) {
	auto slave = NOT_NULL(ENT_UNBOX(ARGS.slave));

	int dir = 1-ARGS.type * 3;
	for(int t = 0;; t++) {
		slave->pos = global.boss->pos + 100 * cdir(0.05*dir*t);
		YIELD;
	}
}

TASK(lambda_calculus_slave, { cmplx pos; int type; LambdaAbstractionArray *lambdas; }) {
	auto slave = TASK_BIND(stagex_host_yumemi_slave(ARGS.pos, ARGS.type));
	INVOKE_SUBTASK(lambda_calculus_slave_move, ENT_BOX(slave), ARGS.type);

	WAIT(40);
	for(;;) {

		for(int i = -1; i <= 1; i++) {
			cmplx offset = cdir(0.1*i);
			cmplx vel = 3*(offset*global.plr.pos - slave->pos)/cabs(global.plr.pos - global.boss->pos);
			LambdaAbstraction ld = random_abstraction(7, global.frames*global.frames + ARGS.type*124);
			auto bullet = spawn_lambda_bullet(&ld, slave->pos, move_linear(vel));
			ld.ref = ENT_BOX(bullet);
			lambda_array_add(ARGS.lambdas, ld);

			WAIT(20);
		}

		WAIT(100);
	}
	
}

DEFINE_EXTERN_TASK(stagex_spell_lambda_calculus) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	LambdaAbstractionArray lambdas = {};
	dynarray_ensure_capacity(&lambdas, 100);
	INVOKE_SUBTASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, lambda_calculus_cleanup, &lambdas);

	INVOKE_SUBTASK(lambda_array_process, &lambdas);

	INVOKE_TASK(lambda_calculus_slave, boss->pos + 100, 0, &lambdas);
	INVOKE_TASK(lambda_calculus_slave, boss->pos - 100, 1, &lambdas);
	

	for(;;) {
		YIELD;
	}
}
