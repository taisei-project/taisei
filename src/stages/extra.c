/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "extra.h"
#include "coroutine.h"
#include "global.h"
#include "common_tasks.h"

TASK(glider_bullet, {
	complex pos; double dir; double spacing; int interval;
}) {
	const int nproj = 5;
	const int nstep = 4;
	BoxedProjectile projs[nproj];

	const complex positions[][5] = {
		{1+I, -1, 1, -I, 1-I},
		{2, I, 1, -I, 1-I},
		{2, 1+I, 2-I, -I, 1-I},
		{2, 0, 2-I, 1-I, 1-2*I},
	};

	complex trans = cdir(ARGS.dir+1*M_PI/4)*ARGS.spacing;

	for(int i = 0; i < nproj; i++) {
		projs[i] = ENT_BOX(PROJECTILE(
			.pos = ARGS.pos+positions[0][i]*trans,
			.proto = pp_ball,
			.color = RGBA(0,0,1,1),
		));
	}
			

	for(int step = 0;; step++) {
		int cur_step = step%nstep;
		int next_step = (step+1)%nstep;

		int dead_count = 0;
		for(int i = 0; i < nproj; i++) {
			Projectile *p = ENT_UNBOX(projs[i]);
			if(p == NULL) {
				dead_count++;
			} else {
				p->move.retention = 1;
				p->move.velocity = -(positions[cur_step][i]-(1-I)*(cur_step==3)-positions[next_step][i])*trans/ARGS.interval;
			}
		}
		if(dead_count == nproj) {
			return;
		}
		WAIT(ARGS.interval);
	}
}

TASK(glider_fairy, {
	double hp; complex pos; complex dir;
}) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(VIEWPORT_W/2-10*I, ARGS.hp, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.power = 3,
		.points = 5,
	});



	YIELD;

	for(int i = 0; i < 80; ++i) {
		e->pos += cnormalize(ARGS.pos-e->pos)*2;
		YIELD;
	}


	for(int i = 0; i < 3; i++) {
		double aim = carg(global.plr.pos - e->pos);
		INVOKE_TASK(glider_bullet, e->pos, aim-0.7, 20, 6);
		INVOKE_TASK(glider_bullet, e->pos, aim, 25, 3);
		INVOKE_TASK(glider_bullet, e->pos, aim+0.7, 20, 6);
		WAIT(80-20*i);
	}

	for(;;) {
		e->pos += 2*(creal(e->pos)-VIEWPORT_W/2 > 0)-1;
		YIELD;
	}
}

TASK(stage_main, NO_ARGS) {
	YIELD;

	for(int i = 0;;i++) {
		INVOKE_TASK_DELAYED(60, glider_fairy, 2000, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		stage_wait(50+100*(i&1));
	}
}

static void extra_begin(void) {
	INVOKE_TASK(stage_main);
}


StageProcs extra_procs = {
	.begin = extra_begin,
};
