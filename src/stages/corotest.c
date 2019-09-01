/*
 * This software is licensed under the terms of the MIT-License
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

TASK(laserproj_death, { Projectile *p; }) {
	spawn_projectile_clear_effect(ARGS.p);
}

TASK(laserize_proj, { BoxedProjectile p; int t; }) {
	Projectile *p = TASK_BIND(ARGS.p);
	WAIT(ARGS.t);

	complex pos = p->pos;
	double a = p->angle;
	Color clr = p->color;
	kill_projectile(p);

	complex aim = 12 * cexp(I * a);
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
	// WAIT_EVENT yields until the event fires.
	// Returns true if the event was signaled, false if it was canceled.
	// All waiting tasks will resume right after either of those occur, in the
	// order they started waiting.

	Enemy *e = ENT_UNBOX(ARGS.e);

	if(!WAIT_EVENT(&e->events.killed)) {
		// Event canceled? Nothing to do here.
		log_debug("[%p] leave (canceled)", (void*)cotask_active());
		return;
	}

	// Event signaled. Since this is an enemy death event, e will be invalid
	// in the next frame. Let's save its position while we can.
	complex pos = e->pos;

	while(ARGS.rounds--) {
		WAIT(ARGS.delay);

		double angle_ofs = frand() * M_PI * 2;

		for(int i = 0; i < ARGS.cnt; ++i) {
			complex aim = cexp(I * (angle_ofs + M_PI * 2.0 * i / (double)ARGS.cnt));

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

TASK_WITH_FINALIZER(test_enemy, {
	double hp; complex pos; complex dir;
	struct { int x; } for_finalizer;
}) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, ARGS.hp, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.life_fragment = 1,
		.bomb_fragment = 1,
		.power = 3,
		.points = 5,
	});

	INVOKE_TASK(wait_event_test, ENT_BOX(e), 3, 10, 15, 3);

	for(;;) {
		YIELD;
		ARGS.for_finalizer.x++;

		// wander around for a bit...
		for(int i = 0; i < 60; ++i) {
			e->pos += ARGS.dir;
			YIELD;
		}

		// stop and...
		WAIT(10);

		// pew pew!!!
		complex aim = 3 * cnormalize(global.plr.pos - e->pos);
		int pcount = 120;

		for(int i = 0; i < pcount; ++i) {
			for(int j = -1; j < 2; j += 2) {
				double a = j * M_PI * 0.1 * psin(20 * (2 * global.frames + i * 10));

				Projectile *p = PROJECTILE(
					.pos = e->pos,
					.proto = pp_rice,
					.color = RGBA(1.0, 0.0, i / (pcount - 1.0), 0.0),
					.move = move_asymptotic(aim * 4 * cdir(a + M_PI), aim * cdir(-a * 2), 0.9),
					.max_viewport_dist = 128,
				);

				INVOKE_TASK(laserize_proj, ENT_BOX(p), 40);
			}

			WAIT(2);
		}

		// keep wandering, randomly
		ARGS.dir *= cexp(I*M_PI*nfrand());
	}
}

TASK_FINALIZER(test_enemy) {
	log_debug("finalizer called (x = %i)", ARGS.for_finalizer.x);
}

TASK_WITH_FINALIZER(subtask_test, { int depth; int num; }) {
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

TASK_FINALIZER(subtask_test) {
	log_debug("finalize subtask_test: depth=%i; num=%i", ARGS.depth, ARGS.num);
}

TASK(subtask_test_init, NO_ARGS) {
	log_debug("************ BEGIN ************");
	INVOKE_SUBTASK(subtask_test, 3, -1);
	WAIT(30);
	log_debug("************  END  ************");
}

TASK(stage_main, NO_ARGS) {
	YIELD;

	stage_wait(30);
	log_debug("test 1! %i", global.timer);
	stage_wait(60);
	log_debug("test 2! %i", global.timer);

	for(;;) {
		INVOKE_TASK(subtask_test_init);
		INVOKE_TASK_DELAYED(60, test_enemy, 9000, CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5, 3*I);
		stage_wait(1000);
	}
}

static void cotest_begin(void) {
	INVOKE_TASK(stage_main);
}

StageProcs corotest_procs = {
	.begin = cotest_begin,
};
