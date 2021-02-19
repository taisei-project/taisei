/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "corotest.h"
#include "coroutine.h"
#include "global.h"
#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

DIAGNOSTIC(ignored "-Wunused-variable")

TASK(laserproj_death, { Projectile *p; }) {
	spawn_projectile_clear_effect(ARGS.p);
}

TASK(laserize_proj, { BoxedProjectile p; int t; }) {
	Projectile *p = TASK_BIND(ARGS.p);
	WAIT(ARGS.t);

	cmplx pos = p->pos;
	double a = p->angle;
	Color clr = p->color;
	kill_projectile(p);

	cmplx aim = 12 * cexp(I * a);
	create_laserline(pos, aim, 60, 80, &clr);

	p = PROJECTILE(
		.pos = pos,
		.proto = pp_ball,
		.color = &clr,
		.timeout = 60,
	);

	INVOKE_TASK_WHEN(&p->events.killed, laserproj_death, p);
}

TASK(wait_event_test, { BoxedEnemy e; int rounds; int delay; int cnt; int cnt_inc; }) {
	Enemy *e = ENT_UNBOX(ARGS.e);

	if(WAIT_EVENT(&e->events.killed).event_status == CO_EVENT_CANCELED) {
		// Event canceled? Nothing to do here.
		log_debug("[%p] leave (canceled)", (void*)cotask_active());
		return;
	}

	// Event signaled. Since this is an enemy death event, e will be invalid
	// in the next frame. Let's save its position while we can.
	cmplx pos = e->pos;

	while(ARGS.rounds--) {
		WAIT(ARGS.delay);

		real angle_ofs = rng_angle();

		for(int i = 0; i < ARGS.cnt; ++i) {
			cmplx aim = cexp(I * (angle_ofs + M_PI * 2.0 * i / (double)ARGS.cnt));

			PROJECTILE(
				.pos = pos,
				.proto = pp_crystal,
				.color = RGBA(i / (double)ARGS.cnt, 0.0, 1.0 - i / (double)ARGS.cnt, 0.0),
				.move = move_asymptotic(12 * aim, 2 * aim, 0.8),
			);

			WAIT(1);
		}

		ARGS.cnt += ARGS.cnt_inc;
	}
}

TASK(test_enemy, {
	double hp; cmplx pos; cmplx dir;
}) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, ARGS.hp, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.life_fragment = 1,
		.bomb_fragment = 1,
		.power = 3,
		.points = 5,
	});

	INVOKE_TASK(wait_event_test, ENT_BOX(e), 3, 10, 15, 3);

	for(;;) {
		YIELD;

		// wander around for a bit...
		for(int i = 0; i < 60; ++i) {
			e->pos += ARGS.dir;
			YIELD;
		}

		// stop and...
		WAIT(10);

		// pew pew!!!
		cmplx aim = 3 * cnormalize(global.plr.pos - e->pos);
		int pcount = 120;

		for(int i = 0; i < pcount; ++i) {
			for(int j = -1; j < 2; j += 2) {
				double a = j * M_PI * 0.1 * psin(20 * (2 * global.frames + i * 10));

				Projectile *p = PROJECTILE(
					.pos = e->pos,
					.proto = pp_rice,
					.color = RGBA(1.0, 0.2, i / (pcount - 1.0), 0.0),
					.move = move_asymptotic(aim * 4 * cdir(a + M_PI), aim * cdir(-a * 2), 0.9),
					.max_viewport_dist = 128,
				);

				INVOKE_TASK(laserize_proj, ENT_BOX(p), 40);
			}

			WAIT(2);
		}

		// keep wandering, randomly
		ARGS.dir *= rng_dir();
	}
}

TASK(subtask_test, { int depth; int num; }) {
	if(ARGS.depth > 0) {
		for(int i = 0; i < 3; ++i) {
			INVOKE_SUBTASK(subtask_test, ARGS.depth - 1, i);
		}
	}

	for(int i = 0;; ++i) {
		log_debug("subtask_test: depth=%i; num=%i; iter=%i", ARGS.depth, ARGS.num, i);
		YIELD;
	}
}

TASK(subtask_test_init, NO_ARGS) {
	log_debug("************ BEGIN ************");
	INVOKE_SUBTASK(subtask_test, 3, -1);
	WAIT(300);
	log_debug("************  END  ************");
}

TASK(punching_bag, NO_ARGS) {
	Enemy *e = TASK_BIND(create_enemy1c(0.5*(VIEWPORT_W+VIEWPORT_H*I), 1000, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.life_fragment = 10,
		.bomb_fragment = 100,
		.power = 3,
		.points = 5,
	});

	for(;;) {
		INVOKE_TASK(common_charge,
			.pos = e->pos,
			.color = RGBA(1, 0, 0, 0),
			.time = 60
		);
		WAIT(80);
	}
}

TASK(stage_main, NO_ARGS) {
	YIELD;

	/*
	WAIT(30);
	log_debug("test 1! %i", global.timer);
	WAIT(60);
	log_debug("test 2! %i", global.timer);
	*/

	// INVOKE_TASK(punching_bag);
	return;

	for(;;) {
		INVOKE_TASK(subtask_test_init);
		INVOKE_TASK_DELAYED(60, test_enemy, 9000, CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5, 3*I);
		WAIT(1000);
	}
}

static void cotest_begin(void) {
	// corotest_procs.shader_rules = stage1_procs.shader_rules;
	// stage1_procs.begin();
	INVOKE_TASK(stage_main);
}

static void cotest_update(void) {
	// stage1_procs.update();
}

static void cotest_end(void) {
	// stage1_procs.end();
}

static void cotest_draw(void) {
	// stage1_procs.draw();
}

StageProcs corotest_procs = {
	.begin = cotest_begin,
	.end = cotest_end,
	.update = cotest_update,
	.draw = cotest_draw,
};
