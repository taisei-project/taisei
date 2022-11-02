/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage3.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"
#include "enemy_classes.h"

static void stage3_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage3PostBossDialog, pm->dialog->Stage3PostBoss);
}

TASK(swarm_trail_proj, { cmplx pos; cmplx vstart; cmplx vend; real x; real width;}) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_rice,
		.color = RGB(0.4, 0, 0.8),
		.max_viewport_dist = 1000,
	));
	BoxedProjectile phead = ENT_BOX(PROJECTILE(
		.proto = pp_flea,
		.color = RGB(0.8, 0.1, 0.4),
		.max_viewport_dist = 1000,
	));

	real turn_length = 1; // |x/turn_length| should be smaller than pi

	cmplx prevpos = ARGS.pos;
	for(int t = -70;; t++) {
		if(t == 0) {
			play_sfx("redirect");
		}
		cmplx z = t/ARGS.width + I * ARGS.x/turn_length;

		p->pos = ARGS.pos + ARGS.width * z * (ARGS.vstart + (ARGS.vend-ARGS.vstart)/(cexp(-z) + 1));

		p->angle = carg(p->pos-prevpos);

		Projectile *head = ENT_UNBOX(phead);
		if(head) {
			head->pos = p->pos + 8*cdir(p->angle);
		}
		p->move = move_linear(p->pos-prevpos);

		prevpos = p->pos;
		YIELD;
	}
}

TASK(swarm_trail_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(
		.points = 1,
		.power = 2,
	)));

	e->move = ARGS.move;

	int shooting_time = 200;
	real width = 30;
	int interval = 10;
	int nrow = 5;

	WAIT(30);
	e->move.retention = 0.9;
	WAIT(10);
	cmplx aim = cnormalize(global.plr.pos - e->pos);

	for(int t = 0; t < shooting_time/interval; t++) {
		play_sfx("shot1");

		for(int i = 0; i < nrow - (t&1); i++) {
			real x = (i/(real)(nrow-1) - 0.5);

			INVOKE_TASK(swarm_trail_proj, e->pos, ARGS.move.velocity, 3*aim, x, .width = width);
		}
		WAIT(interval);
	}
	play_sfx("redirect");
	e->move = move_asymptotic_halflife(0, -ARGS.move.velocity , 120);
}

TASK(swarm_trail_fairy_spawn, { int count; }) {
	for(int i = 0; i < ARGS.count; i++) {
		cmplx pos = VIEWPORT_W / 2.0;
		cmplx vel = 5*I + 4 * cdir(-M_PI * i / (real)(ARGS.count-1));
		INVOKE_TASK(swarm_trail_fairy, pos, move_linear(vel));

		WAIT(60);
	}
}

TASK(flower_swirl_trail, { BoxedEnemy e; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	for(;;) {
		YIELD;
		PARTICLE(
			.sprite = "smoothdot",
			.color = RGBA(2, 1, 0.6, 0),
			.pos = e->pos,
			.timeout = 30,
			.draw_rule = pdraw_timeout_scalefade_exp(1, 2, 1, 0, 2),
		);
	}
}

static inline MoveParams flower_swirl_move(cmplx dir, real angular_velocity, real radius) {
	cmplx retention = 0.99 * cdir(angular_velocity);
	return (MoveParams) {
		.velocity = radius * cnormalize(dir) * I * cabs(1 - retention),
		.retention = retention,
		.acceleration = dir * (1 - retention),
	};
}

TASK(flower_swirl, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(
		.power = 1,
	)));

	e->move = ARGS.move;
	INVOKE_TASK(flower_swirl_trail, ENT_BOX(e));

	cmplx twist = cdir(M_TAU/64);

	int interval = 20;
	int petals = 5;
	Color pcolor = *RGBA(1, 0.5, 0, 1);

	for(int t = 0;; t++) {
		if(t & 1) {
			pcolor.g = 0;
			petals = 7;
		} else {
			pcolor.g = 0.5;
			petals = 5;
		}

		play_sfx("shot2");

		cmplx r = cnormalize(e->move.velocity);

		for(int i = 0; i < petals; i++) {
			cmplx dir = r * cdir(M_TAU / petals * i);
			PROJECTILE(
				.proto = pp_wave,
				.color = &pcolor,
				.pos = e->pos - 4 * dir,
				.move = move_asymptotic_halflife(0.7*dir, -4 * dir * twist, 200),
			);
		}

		WAIT(interval);
	}
}

TASK(flower_swirl_spawn, { cmplx pos; MoveParams move; int count; int interval; }) {
	for(int i = 0; i < ARGS.count; i++) {
		INVOKE_TASK(flower_swirl, ARGS.pos, ARGS.move);
		WAIT(ARGS.interval);
	}
}

TASK(horde_fairy_motion, { BoxedEnemy e; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	real ofs = rng_angle();
	cmplx v = e->move.velocity;
	for(;;YIELD) {
		e->move.velocity = v * cdir(M_PI/8 * sin(ofs + global.frames/15.0));
	}
}

TASK(horde_fairy, { cmplx pos; bool blue; }) {
	Enemy *e;
	if(ARGS.blue) {
		e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(
			.points = 1,
		)));
	} else {
		e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
			.power = 1,
		)));
	}

	e->move = move_linear(2 * I);

	INVOKE_SUBTASK(horde_fairy_motion, ENT_BOX(e));

	int interval = 40;

	for(;;WAIT(interval)) {
		play_sfx("shot1");
		cmplx diff = global.plr.pos - e->pos;

		if(cabs(diff) < 40) {
			continue;
		}

		cmplx aim = cnormalize(diff);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGB(0.4, 0, 1),
			.move = move_asymptotic_halflife(0, 2 * aim, 25),
		);
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.1, 0.4, 0.8),
			.move = move_asymptotic_halflife(0, 2 * aim, 20),
		);
	}
}

TASK(horde_fairy_spawn, { int count; int interval; }) {
	real p = 3.0 / ARGS.count;
	real w = 0.9*VIEWPORT_W/2;
	bool blue = rng_bool();
	for(int t = 0; t < ARGS.count; t++) {
		INVOKE_TASK(horde_fairy,
			.pos = VIEWPORT_W/2 + w * triangle(t * p),
			.blue = blue,
		);
		if(((t+1) % 3) == 0) {
			blue = !blue;
		}
		WAIT(ARGS.interval);
	}
}

TASK(circle_twist_fairy_lances, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	common_charge(120, &e->pos, 0, RGBA(0.6, 0.8, 2, 0));
	cmplx offset = rng_dir();

	int lance_count = 300;
	int lance_segs = 20;
	int lance_dirs = 100;
	int fib1 = 1;
	int fib2 = 1;
	for(int i = 0; i < lance_count; i++) {
		play_sfx("shot3");
		int tmp = fib1;
		fib1 = (fib1 + fib2) % lance_dirs;
		fib2 = tmp;

		cmplx dir = offset * cdir(M_TAU / lance_dirs * fib1);
		for(int j = 0; j < lance_segs; j++) {
			int s = 1 - 2 * (j & 1);
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos + 20 * dir,
				.color = RGB(0.3, 0.4, 1),
					.move = move_asymptotic_halflife((0.5+4.5*j/lance_segs)*dir, 5*dir*cdir(0.05 * s), 50+10*I),
			);
		}

		WAIT(2);
	}
}

TASK(circle_twist_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(
		.power = 5,
		.points = 5,
		.bomb_fragment = 1,
	)));

	e->move=move_towards(ARGS.target_pos, 0.01);
	WAIT(50);

	int circle_count = 10;
	int count = 50;
	int interval = 40;

	INVOKE_SUBTASK_DELAYED(180, circle_twist_fairy_lances, ENT_BOX(e));

	for(int i = 0; i < circle_count; i++) {
		int s = 1-2*(i&1);
		play_sfx("shot_special1");
		for(int t = 0; t < count; t++) {
			play_sfx_loop("shot1_loop");
			cmplx dir = -I * cdir(s * M_TAU / count * t);
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGB(1, 0.3, 0),
				.move = {
					.velocity = (2 + 0.005 * t) * dir,
					.acceleration = -0.005 * dir,
					.retention = 1.0 * cdir(-0.01*s)
				}
			);
			YIELD;
		}
		WAIT(interval);
	}

	e->move = move_asymptotic_halflife(0, 2 * I, 20);

}

TASK(laserball, { cmplx origin; cmplx velocity; Color *color; real freq_factor; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_bigball,
		.pos = ARGS.origin,
		.move = move_asymptotic_halflife(ARGS.velocity, 0, 15),
		.color = ARGS.color,
	));

	WAIT(60);

	cmplx lv = -4 * cnormalize(ARGS.velocity);

	real lt = 20;
	real dt = 300;

	real amp = 0.08;
	real freq = 0.25 * ARGS.freq_factor;

	int charges = 4;
	int delay = 20;
	int refire = 20;

	real phase = 0;

	real scale = 1;
	real scale_per_charge = 1 / (real)charges;
	real scale_per_tick = scale_per_charge / delay;

	cmplx orig_collision = p->collision_size;
	cmplx orig_scale = p->scale;

	for(int i = 0;; ++i) {
		play_sfx("laser1");
		PROJECTILE(
			.pos = p->pos,
			.proto = pp_flea,
			.timeout = 1,
			.color = RGBA(1, 1, 1, 0),
		);
		Laser *l = create_lasercurve4c(p->pos, lt, dt, &p->color, las_sine_expanding, lv, amp, freq, i * phase);

		for(int t = 0; t < delay; ++t) {
			YIELD;
			scale -= scale_per_tick;
			p->collision_size = orig_collision * scale;
			p->scale = orig_scale * scale;		}

		if(i == charges - 1) {
			break;
		}

		WAIT(refire);
		// phase += 3.2;
		freq *= -1;
	}

	kill_projectile(p);
}

TASK(laserball_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(
		.power = 5,
		.points = 5,
	)));

	e->move = move_towards(ARGS.target_pos, 0.04);
	WAIT(30);
	common_charge(90, &e->pos, 0, RGBA(0.5, 1, 0.25, 0));

	int balls = 6;
	int cycles = 2;

	for(int c = 0; c < cycles; ++c) {
		play_sfx("shot1");
		play_sfx("warp");

		for(int i = 0; i < balls; ++i) {
			cmplx v = 3 * cdir(i * (M_TAU / balls));
			INVOKE_TASK(laserball, e->pos, v, RGBA(0.2, 2, 0.4, 0), 1);
		}

		WAIT(120);
		play_sfx("shot1");
		play_sfx("warp");

		for(int i = 0; i < balls; ++i) {
			cmplx v = 3 * cdir((i + 0.5) * (M_TAU / balls));
			INVOKE_TASK(laserball, e->pos, v, RGBA(2, 1, 0.4, 0), -1);
		}

		WAIT(180);
	}

	e->move = move_asymptotic_halflife(e->move.velocity, 3*I, 60);
}

// bosses

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	boss->move = move_towards(VIEWPORT_W/2 + 100.0*I, 0.04);
}

TASK_WITH_INTERFACE(midboss_outro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(int t = 0;; t++) {
		boss->pos += t*t/900.0 * cdir(3*M_PI/2 + 0.5 * sin(t / 20.0));
		YIELD;
	}
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);
	boss_add_attack_task(boss, AT_Move, "Introduction", 1, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Lethal Bite", 11, 20000, TASK_INDIRECT(BossAttack, stage3_midboss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack_task(boss, AT_Move, "Runaway", 2, 1, TASK_INDIRECT(BossAttack, midboss_outro), NULL);
	boss->zoomcolor = *RGB(0.4, 0.1, 0.4);

	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK_DELAYED(120, boss);

	Boss *boss = global.boss = stage3_spawn_wriggle(VIEWPORT_W/2 - 200.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage3PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage3PreBossDialog, pm->dialog->Stage3PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "", 11, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.wriggle_night_ignite, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(boss, &stage3_spells.extra.light_singularity, false);

	boss_engage(boss);
}

TASK(flower_swirls_alternating) {
	int interval = 20;
	int cnt = 4;
	int altinterval = interval * cnt * 2;

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-2 + 0.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = - 10 + 160 * I,
		.move = flower_swirl_move(2 + 0.0 * I, -0.09, 120),
		.count = 4,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-1 + 1.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = - 10 + 160 * I,
		.move = flower_swirl_move(1 + 1.0 * I, -0.09, 120),
		.count = 4,
		.interval = interval,
	);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-2 + 0.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);
}

DEFINE_EXTERN_TASK(stage3_timeline) {
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);

/*
	INVOKE_TASK(laserball_fairy, VIEWPORT_W/2, VIEWPORT_W/2 + VIEWPORT_H/3*I);
	INVOKE_TASK_DELAYED(800, common_call_func, stage_load_quicksave);
	return;*/

	int interval = 60;
	int lr_stagger = 0;
	int ud_stagger = interval / 2;

	INVOKE_TASK_DELAYED(0, flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 200 * I,
		.move = flower_swirl_move(-3 + 0.5 * I, -0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(ud_stagger, flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 200 * I,
		.move = flower_swirl_move(-3 - 1 * I, 0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(lr_stagger, flower_swirl_spawn,
		.pos = -10 + 200 * I,
		.move = flower_swirl_move(3 + 0.5 * I, 0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(lr_stagger + ud_stagger, flower_swirl_spawn,
		.pos = -10 + 200 * I,
		.move = flower_swirl_move(3 - 1 * I, -0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(400, swarm_trail_fairy_spawn, 5);

	STAGE_BOOKMARK_DELAYED(800, horde);
	INVOKE_TASK_DELAYED(800, horde_fairy_spawn,
		.count = difficulty_value(20, 20, 30, 30),
		.interval = 20
	);

	INVOKE_TASK_DELAYED(1000, laserball_fairy,
		.pos = VIEWPORT_W + 10 + 300 * I,
		.target_pos = 3*VIEWPORT_W/4 + 200*I
	);

	STAGE_BOOKMARK_DELAYED(1300, circle-twist);
	INVOKE_TASK_DELAYED(1300, circle_twist_fairy,
		.pos = 0,
		.target_pos = VIEWPORT_W/2.0 + I*VIEWPORT_H/3.0,
	);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(1820, laserball_fairy,
			.pos = - 10 + 300 * I,
			.target_pos = VIEWPORT_W/3 + 140*I
		);

		INVOKE_TASK_DELAYED(1820, laserball_fairy,
			.pos = VIEWPORT_W + 10 + 300 * I,
			.target_pos = 2*VIEWPORT_W/3 + 140*I
		);
	}

	INVOKE_TASK_DELAYED(1900, flower_swirls_alternating);
	INVOKE_TASK_DELAYED(2390, swarm_trail_fairy, VIEWPORT_W+20 + VIEWPORT_H*0.24*I, move_linear(-9));
	INVOKE_TASK_DELAYED(2450, swarm_trail_fairy,           -20 + VIEWPORT_H*0.32*I, move_linear( 9));

	INVOKE_TASK_DELAYED(3400, common_call_func, stage_load_quicksave);

	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);

	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(150);

	STAGE_BOOKMARK(post-midboss);

	STAGE_BOOKMARK_DELAYED(2800, pre-boss);

	WAIT(3000);

	stage_unlock_bgm("stage3boss");

	INVOKE_TASK(spawn_boss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(240);
	stage3_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
