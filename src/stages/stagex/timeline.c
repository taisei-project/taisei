/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "timeline.h"
#include "yumemi.h"
#include "stagex.h"
#include "draw.h"
#include "stage.h"
#include "global.h"
#include "common_tasks.h"
#include "enemy_classes.h"

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

static void yumemi_slave_aimed_burst(YumemiSlave *slave, int count, real *a, real s) {
	for(int burst = 0; burst < count; ++burst) {
		int cnt = 3;

		for(int i = 0; i < cnt; ++i) {
			play_sfx_loop("shot1_loop");

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

static void yumemi_slave_aimed_funnel(YumemiSlave *slave, int count, real *a, real s) {
	real max_angle = M_PI/2;
	real min_angle = M_PI/24;

	for(int burst = 0; burst < count; ++burst) {
		real bf = burst / (count - 1.0);
		cmplx rot = cdir(lerp(max_angle, min_angle, bf) * 0.03);

		int cnt = 3;
		for(int i = 0; i < cnt; ++i) {
			play_sfx_loop("shot1_loop");

			cmplx pos = slave->pos + 32 * cdir(s * (*a + i*M_TAU/cnt));
			cmplx aim = cnormalize(global.plr.pos - pos);
			aim *= 10;

			PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(1, 0.4, 1 - bf, 0),
				.pos = pos,
				// .move = move_asymptotic_simple(aim * rot, -2),
				.move = { .velocity = aim, .retention = rot, },
			);

			PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(1, 0.4, 1 - bf, 0),
				.pos = pos,
				// .move = move_asymptotic_simple(aim / rot, -2),
				.move = { .velocity = aim, .retention = 1/rot, },
			);

			YIELD;
		}

		*a += M_PI/32;
	}

	WAIT(20);
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
		yumemi_slave_aimed_burst(slave, 25, &a, s);
		yumemi_slave_aimed_lasers(slave, s);
		WAIT(40);
	}
}

TASK_WITH_INTERFACE(yumemi_opening, BossAttack) {
	STAGE_BOOKMARK(non1);

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

	WAIT_EVENT_ONCE(&ARGS.attack->events.initiated);
	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	STALL;
}

TASK(yumemi_non2_slave, {
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
		yumemi_slave_aimed_funnel(slave, 20, &a, s);
		WAIT(10);
		yumemi_slave_aimed_lasers(slave, s);
		WAIT(40);
		// yumemi_slave_aimed_burst(slave, 15, &a, s);
	}
}

TASK_WITH_INTERFACE(yumemi_non2, BossAttack) {
	STAGE_BOOKMARK(non2);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non2_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non2_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
	STALL;
}

TASK(yumemi_non3_slave, {
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
		yumemi_slave_aimed_funnel(slave, 20, &a, s);
		WAIT(10);
		yumemi_slave_aimed_lasers(slave, s);
		WAIT(40);
		yumemi_slave_aimed_burst(slave, 15, &a, s);
	}
}

TASK_WITH_INTERFACE(yumemi_non3, BossAttack) {
	STAGE_BOOKMARK(non3);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non3_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non3_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
	STALL;
}

TASK(yumemi_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
}

TASK(boss, { Boss **out_boss; }) {
	STAGE_BOOKMARK(boss);

	Player *plr = &global.plr;
	PlayerMode *pm = plr->mode;

	Boss *boss = stagex_spawn_yumemi(5*VIEWPORT_W/4 - 200*I);
	*ARGS.out_boss = global.boss = boss;

#if 0
	Attack *opening_attack = boss_add_attack(boss, AT_Normal, "Opening", 60, 40000, NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.sierpinski, false);
	boss_add_attack_from_info(boss, &stagex_spells.boss.infinity_network, false);

	StageExPreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, yumemi_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");
	INVOKE_TASK_WHEN(&e->music_changes, yumemi_opening, ENT_BOX(boss), opening_attack);
	WAIT_EVENT_OR_DIE(&global.dialog->events.fadeout_began);
#else
	boss_add_attack_task(boss, AT_Normal, "non1", 60, 40000, TASK_INDIRECT(BossAttack, yumemi_opening), NULL);
	boss_add_attack_task(boss, AT_Normal, "non2", 60, 40000, TASK_INDIRECT(BossAttack, yumemi_non2), NULL);
	boss_add_attack_task(boss, AT_Normal, "non3", 60, 40000, TASK_INDIRECT(BossAttack, yumemi_non3), NULL);
#endif

	boss_engage(boss);
	WAIT_EVENT_OR_DIE(&boss->events.defeated);
	WAIT(240);

	INVOKE_TASK_INDIRECT(StageExPostBossDialog, pm->dialog->StageExPostBoss);
	WAIT_EVENT_OR_DIE(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}

DEFINE_EXTERN_TASK(stagex_timeline) {
	YIELD;

	// goto enemies;

	// WAIT(3900);
	stagex_get_draw_data()->tower_global_dissolution = 1;

	Boss *boss;
	INVOKE_TASK(boss, &boss);
	STALL;

attr_unused enemies:
	for(int i = 0;;i++) {
		INVOKE_TASK_DELAYED(60, glider_fairy, 2000, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(50+100*(i&1));
	}
}
