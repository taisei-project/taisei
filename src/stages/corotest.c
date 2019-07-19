/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "corotest.h"
#include "coroutine.h"
#include "global.h"

static CoSched *cotest_sched;

static void cotest_stub_proc(void) { }

TASK(drop_items, { complex *pos; ItemCounts items; }) {
	complex p = *ARGS.pos;

	for(int i = 0; i < ITEM_LAST - ITEM_FIRST; ++i) {
		for(int j = ARGS.items.as_array[i]; j; --j) {
			spawn_item(p, i + ITEM_FIRST);
			WAIT(2);
		}
	}
}

TASK(laserize_proj, { Projectile *p; int t; }) {
	TASK_BIND(ARGS.p);
	WAIT(ARGS.t);

	complex pos = ARGS.p->pos;
	double a = ARGS.p->angle;
	Color clr = ARGS.p->color;
	kill_projectile(ARGS.p);

	complex aim = 12 * cexp(I * a);
	create_laserline(pos, aim, 20, 40, &clr);

	PROJECTILE(
		.pos = pos,
		.proto = pp_ball,
		.color = &clr,
		.timeout = 20,
	);
}

TASK(wait_event_test, { Enemy *e; int rounds; int delay; int cnt; int cnt_inc; }) {
	// WAIT_EVENT yields until the event fires.
	// Returns true if the event was signaled, false if it was canceled.
	// All waiting tasks will resume right after either of those occur, in the
	// order they started waiting.

	if(!WAIT_EVENT(&ARGS.e->events.killed)) {
		// Event canceled? Nothing to do here.
		log_debug("[%p] leave (canceled)", (void*)cotask_active());
		return;
	}

	// Event signaled. Since this is an enemy death event, e will be invalid
	// in the next frame. Let's save its position while we can.
	complex pos = ARGS.e->pos;

	while(ARGS.rounds--) {
		WAIT(ARGS.delay);

		double angle_ofs = frand() * M_PI * 2;

		for(int i = 0; i < ARGS.cnt; ++i) {
			complex aim = cexp(I * (angle_ofs + M_PI * 2.0 * i / (double)ARGS.cnt));

			PROJECTILE(
				.pos = pos,
				.proto = pp_crystal,
				.color = RGBA(i / (double)ARGS.cnt, 0.0, 1.0 - i / (double)ARGS.cnt, 0.0),
				.rule = asymptotic,
				.args = { 2 * aim, 5 },
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
	Enemy *e = create_enemy1c(ARGS.pos, ARGS.hp, BigFairy, NULL, 0);
	TASK_BIND(e);

	INVOKE_TASK_WHEN(&e->events.killed, drop_items, &e->pos, {
		.life_fragment = 1,
		.bomb_fragment = 1,
		.power = 3,
		.points = 5,
	});

	INVOKE_TASK(wait_event_test, e, 3, 10, 15, 3);

	for(;;) {
		YIELD;
		ARGS.for_finalizer.x++;

		// wander around for a bit...
		for(int i = 0; i < 20; ++i) {
			e->pos += ARGS.dir;
			YIELD;
		}

		// stop and...
		WAIT(10);

		// pew pew!!!
		int pcount = 10 + 10 * frand();
		for(int i = 0; i < pcount; ++i) {
			complex aim = global.plr.pos - e->pos;
			aim *= 5 * cexp(I*M_PI*0.1*nfrand()) / cabs(aim);

			Projectile *p = PROJECTILE(
				.pos = e->pos,
				.proto = pp_rice,
				.color = RGBA(1.0, 0.0, i / (pcount - 1.0), 0.0),
				.rule = linear,
				.args = { aim },
			);

			INVOKE_TASK(laserize_proj, p, 30);

			WAIT(3);
		}

		// keep wandering, randomly
		ARGS.dir *= cexp(I*M_PI*nfrand());
	}
}

TASK_FINALIZER(test_enemy) {
	log_debug("finalizer called (x = %i)", ARGS.for_finalizer.x);
}

TASK(stage_main, { int ignored; }) {
	YIELD;

	WAIT(30);
	log_debug("test 1! %i", global.timer);
	WAIT(60);
	log_debug("test 2! %i", global.timer);

	for(;;) {
		INVOKE_TASK(test_enemy, 9000, CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5, 3*I);
		WAIT(500);
	}
}

static void cotest_begin(void) {
	cotest_sched = cosched_new();
	cosched_set_invoke_target(cotest_sched);
	INVOKE_TASK(stage_main, 0);
}

static void cotest_end(void) {
	cosched_free(cotest_sched);
}

static void cotest_events(void) {
	if(!global.gameover && !cosched_run_tasks(cotest_sched)) {
		log_debug("over!");
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}

StageProcs corotest_procs = {
	.begin = cotest_begin,
	.preload = cotest_stub_proc,
	.end = cotest_end,
	.draw = cotest_stub_proc,
	.update = cotest_stub_proc,
	.event = cotest_events,
	.shader_rules = (ShaderRule[]) { NULL },
};
