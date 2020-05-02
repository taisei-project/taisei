/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"   // IWYU pragma: keep
#include "stagex.h"
#include "yumemi.h"

#include "stages/common_imports.h"

TASK(glider_bullet, {
	cmplx pos; real dir; real spacing; int interval;
}) {
	const int nproj = 5;
	const int nstep = 4;
	BoxedProjectile projs[nproj];

	const cmplx positions[][5] = {
		{1+I, -1, 1, -I, 1-I},
		{2, I, 1, -I, 1-I},
		{2, 1+I, 2-I, -I, 1-I},
		{2, 0, 2-I, 1-I, 1-2*I},
	};

	cmplx trans = cdir(ARGS.dir+1*M_PI/4)*ARGS.spacing;

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
	real hp; cmplx pos; cmplx dir;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(VIEWPORT_W/2-10*I, ITEMS(
		.power = 3,
		.points = 5,
	)));

	YIELD;

	for(int i = 0; i < 80; ++i) {
		e->pos += cnormalize(ARGS.pos-e->pos)*2;
		YIELD;
	}

	for(int i = 0; i < 3; i++) {
		real aim = carg(global.plr.pos - e->pos);
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

static void yumemi_slave_aimed_burst(YumemiSlave *slave, real *a, real s) {
	for(int burst = 0; burst < 25; ++burst) {
		int cnt = 3;
		for(int i = 0; i < cnt; ++i) {
			cmplx pos = slave->pos + 32 * cdir(s * (*a + i*M_TAU/cnt));
			cmplx aim = cnormalize(global.plr.pos - pos);
			aim *= 10;

			PROJECTILE(
				.proto = pp_rice,
				.color = RGBA(1, 0, 0.25, 0),
				.pos = pos,
				.move = move_asymptotic_simple(aim, -2),
			);

			YIELD;
		}

		*a += M_PI/32;
	}

	WAIT(20);
}

static void yumemi_slave_aimed_lasers(YumemiSlave *slave, real s) {
	int cnt = 32;
	int t = 2;
	real g = carg(global.plr.pos - slave->pos);

	INVOKE_TASK_DELAYED(50, common_play_sfx, "laser1");

	for(int i = 0; i < cnt; ++i) {
		real x = i/(real)cnt;
		cmplx o = 32 * cdir(s * x * M_TAU + g + M_PI/2);
		cmplx pos = slave->pos + o;
		cmplx aim = cnormalize(global.plr.pos - pos + o);
		// cmplx aim = I;
		Color *c = RGBA(0.1 + 0.9 * x * x, 1 - 0.9 * (1 - pow(1 - x, 2)), 0.1, 0);
		create_laserline(pos, 40 * aim, 60 + i * t, 80 + i * t, c);
	}

	WAIT(10 + t * cnt);
}

TASK(yumemi_opening_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(;;) {
		yumemi_slave_aimed_burst(slave, &a, s);
		yumemi_slave_aimed_lasers(slave, s);
		WAIT(40);
	}
}

TASK_WITH_INTERFACE(yumemi_opening, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_opening_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(40, yumemi_opening_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	STALL;
}

DEFINE_EXTERN_TASK(stagex_timeline) {
	YIELD;

	// goto enemies;

	WAIT(3900);
	STAGE_BOOKMARK(boss);

	player_set_power(&global.plr, 400);

	Boss *boss = stagex_spawn_yumemi(VIEWPORT_W/2 + 180*I);
	global.boss = boss;

	boss_add_attack_from_info(boss, &stagex_spells.boss.sierpinski, false);
	boss_engage(boss);
	WAIT_EVENT(&boss->events.defeated);
	return;

	Attack *opening_attack = boss_add_attack(boss, AT_Normal, "Opening", 60, 40000, NULL);

	boss_add_attack_from_info(boss, &stagex_spells.boss.infinity_network, false);

	PlayerMode *pm = global.plr.mode;
	StageExPreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");
	INVOKE_TASK_WHEN(&e->music_changes, yumemi_opening, ENT_BOX(boss), opening_attack);

	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_engage(boss);

	WAIT_EVENT(&boss->events.defeated);

enemies:
	for(int i = 0;;i++) {
		INVOKE_TASK_DELAYED(60, glider_fairy, 2000, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(50+100*(i&1));
	}
}
