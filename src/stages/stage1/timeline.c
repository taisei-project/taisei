/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage1.h"
#include "cirno.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"

TASK(burst_fairy, { cmplx pos; cmplx dir; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 700, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
		.power = 1,
	});

	e->move.attraction_point = ARGS.pos + 120*I;
	e->move.attraction = 0.03;

	WAIT(60);

	play_sfx("shot1");
	int n = 1.5 * global.diff - 1;

	for(int i = -n; i <= n; i++) {
		cmplx aim = cdir(carg(global.plr.pos - e->pos) + 0.2 * i);

		PROJECTILE(
			.proto = pp_crystal,
			.pos = e->pos,
			.color = RGB(0.2, 0.3, 0.5),
			.move = move_asymptotic_simple(aim * (2 + 0.5 * global.diff), 5),
		);
	}

	WAIT(1);

	e->move.attraction = 0;
	e->move.acceleration = 0.04 * ARGS.dir;
	e->move.retention = 1;
}

TASK(circletoss_shoot_circle, { BoxedEnemy e; int duration; int interval; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	int cnt = ARGS.duration / ARGS.interval;
	double angle_step = M_TAU / cnt;

	for(int i = 0; i < cnt; ++i) {
		play_sfx_loop("shot1_loop");
		e->move.velocity *= 0.8;

		cmplx aim = cdir(angle_step * i);

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.move = move_asymptotic_simple(2 * aim, i * 0.5),
		);

		WAIT(ARGS.interval);
	}
}

TASK(circletoss_shoot_toss, { BoxedEnemy e; int times; int duration; int period; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	while(ARGS.times--) {
		for(int i = ARGS.duration; i--;) {
			play_sfx_loop("shot1_loop");

			double aim_angle = carg(global.plr.pos - e->pos);
			aim_angle += 0.05 * global.diff * rng_real();

			cmplx aim = cdir(aim_angle);
			aim *= rng_range(1, 3);

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.move = move_asymptotic_simple(aim, 3),
			);

			WAIT(1);
		}

		WAIT(ARGS.period - ARGS.duration);
	}
}

TASK(circletoss_fairy, { cmplx pos; cmplx velocity; cmplx exit_accel; int exit_time; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1500, BigFairy, NULL, 0));

	e->move = move_linear(ARGS.velocity);

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
		.power = 1,
	});

	INVOKE_SUBTASK_DELAYED(60, circletoss_shoot_circle, ENT_BOX(e),
		.duration = 40,
		.interval = 2 + (global.diff < D_Hard)
	);

	if(global.diff > D_Easy) {
		INVOKE_SUBTASK_DELAYED(90, circletoss_shoot_toss, ENT_BOX(e),
			.times = 4,
			.period = 150,
			.duration = 5 + 7 * global.diff
		);
	}

	WAIT(ARGS.exit_time);
	e->move.acceleration += ARGS.exit_accel;
	STALL;
}

TASK(sinepass_swirl_move, { BoxedEnemy e; cmplx v; cmplx sv; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	cmplx sv = ARGS.sv;
	cmplx v = ARGS.v;

	for(;;) {
		sv -= cimag(e->pos - e->pos0) * 0.03 * I;
		e->pos += sv * 0.4 + v;
		YIELD;
	}
}

TASK(sinepass_swirl, { cmplx pos; cmplx vel; cmplx svel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
	});

	INVOKE_TASK(sinepass_swirl_move, ENT_BOX(e), ARGS.vel, ARGS.svel);

	WAIT(difficulty_value(25, 20, 15, 10));

	int shot_interval = difficulty_value(120, 40, 30, 20);

	for(;;) {
		play_sfx("shot1");

		cmplx aim = cnormalize(global.plr.pos - e->pos);
		aim *= difficulty_value(2, 2, 2.5, 3);

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_asymptotic_simple(aim, 5),
		);

		WAIT(shot_interval);
	}
}

TASK(circle_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1400, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.005;
	e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(120);

	int shot_interval = 2;
	int shot_count = difficulty_value(10, 10, 20, 25);
	int round_interval = 120 - shot_interval * shot_count;

	for(int round = 0; round < 2; ++round) {
		double a_ofs = rng_angle();

		for(int i = 0; i < shot_count; ++i) {
			cmplx aim;

			aim = circle_dir_ofs((round & 1) ? i : shot_count - i, shot_count, a_ofs);
			aim *= difficulty_value(1.7, 2.0, 2.5, 2.5);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.6, 0.2, 0.7),
				.move = move_asymptotic_simple(aim, i * 0.5),
			);

			play_sfx_loop("shot1_loop");
			WAIT(shot_interval);
		}

		e->move.attraction_point += 30 * rng_dir();
		WAIT(round_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = -0.04 * I * cdir(rng_range(0, M_TAU / 12));
	STALL;
}

TASK(drop_swirl, { cmplx pos; cmplx vel; cmplx accel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
	});

	e->move = move_accelerated(ARGS.vel, ARGS.accel);

	int shot_interval = difficulty_value(120, 60, 30, 20);

	WAIT(difficulty_value(40, 30, 20, 20));

	while(true) {
		cmplx aim = cnormalize(global.plr.pos - e->pos);
		aim *= 1 + 0.3 * global.diff + rng_real();

		play_sfx("shot1");
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_linear(aim),
		);

		WAIT(shot_interval);
	}
}

TASK(multiburst_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1000, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.05;
	// e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(60);

	int burst_interval = difficulty_value(22, 20, 18, 16);
	int bursts = 4;

	for(int i = 0; i < bursts; ++i) {
		play_sfx("shot1");
		int n = global.diff - 1;

		for(int j = -n; j <= n; j++) {
			cmplx aim = cdir(carg(global.plr.pos - e->pos) + j / 5.0);
			aim *= 2.5;

			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(0.2, 0.3, 0.5),
				.move = move_linear(aim),
			);
		}

		WAIT(burst_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(instantcircle_fairy_shoot, { BoxedEnemy e; int cnt; double speed; double boost; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	play_sfx("shot_special1");

	for(int i = 0; i < ARGS.cnt; ++i) {
		cmplx vel = ARGS.speed * circle_dir(i, ARGS.cnt);

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.move = move_asymptotic_simple(vel, ARGS.boost),
		);
	}
}

TASK(instantcircle_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1200, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
		.power = 4,
	});

	e->move = move_towards(ARGS.target_pos, 0.04);
	BoxedEnemy be = ENT_BOX(e);

	INVOKE_TASK_DELAYED(75, instantcircle_fairy_shoot, be,
		.cnt = difficulty_value(20, 22, 24, 28),
		.speed = 1.5,
		.boost = 2.0
	);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(95, instantcircle_fairy_shoot, be,
			.cnt = difficulty_value(0, 26, 29, 32),
			.speed = 3,
			.boost = 3.0
		);
	}

	WAIT(200);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(waveshot, { cmplx pos; real angle; real spread; real freq; int shots; int interval; } ) {
	for(int i = 0; i < ARGS.shots; ++i) {
		cmplx v = 4 * cdir(ARGS.angle + ARGS.spread * triangle(ARGS.freq * i));

		play_sfx_loop("shot1_loop");
		PROJECTILE(
			.proto = pp_thickrice,
			.pos = ARGS.pos,
			.color = RGBA(0.0, 0.5 * (1.0 - i / (ARGS.shots - 1.0)), 1.0, 1),
			.move = move_asymptotic(-4 * v, v, 0.9),
			// .move = move_accelerated(-v, 0.02 * v),
		);

		WAIT(ARGS.interval);
	}
}

TASK(waveshot_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 4200, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 4,
		.power = 2,
	});

	e->move = move_towards(ARGS.target_pos, 0.03);

	WAIT(120);

	cmplx orig_pos = e->pos;
	real angle = carg(global.plr.pos - orig_pos);
	cmplx pos = orig_pos - 24 * cdir(angle);

	real spread = difficulty_value(M_PI/20, M_PI/18, M_PI/16, M_PI/14);
	real interval = difficulty_value(3, 2, 1, 1);
	real shots = 60 / interval;
	real frequency = 60 * (1.0/12.0) / shots;
	shots += 1;

	INVOKE_SUBTASK(waveshot, pos, angle, rng_sign() * spread, frequency, shots, interval);

	WAIT(120);

	e->move.attraction = 0;
	e->move.retention = 0.8;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(explosion_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 6000, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 8,
	});

	e->move = move_towards(ARGS.target_pos, 0.06);

	WAIT(40);

	INVOKE_SUBTASK(common_charge, 0, RGBA(1.0, 0, 0.2, 0), 60,
		.anchor = &e->pos,
		.sound = COMMON_CHARGE_SOUNDS
	);

	WAIT(60);

	int cnt = difficulty_value(30, 30, 60, 60);
	int trails = difficulty_value(0, 2, 3, 4);
	real speed = difficulty_value(2, 2, 4, 4);
	real ofs = rng_angle();

	play_sfx("shot_special1");

	for(int i = 0; i < cnt; ++i) {
		cmplx aim = speed * circle_dir_ofs(i, cnt, ofs);
		real s = 0.5 + 0.5 * triangle(0.1 * i);

		Color clr;

		if(s == 1) {
			clr = *RGB(1, 0, 0);
		} else {
			clr = *color_lerp(
				RGB(0.1, 0.6, 1.0),
				RGB(1.0, 0.0, 0.3),
				s * s
			);
			color_mul(&clr, &clr);
		}

		PROJECTILE(
			.proto = s == 1 ? pp_bigball : pp_ball,
			.pos = e->pos,
			.color = &clr,
			.move = move_asymptotic_simple(aim, 1 + 8 * s),
		);

		for(int j = 0; j < trails; ++j) {
			aim *= 0.8;
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = &clr,
				.move = move_asymptotic_simple(aim, 1 + 8 * s),
			);
		}
	}

	WAIT(10);

	e->move.attraction = 0;
	e->move.retention = 0.8;
	e->move.acceleration = ARGS.exit_accel;
}

// opening. projectile bursts
TASK(burst_fairies_1, NO_ARGS) {
	for(int i = 3; i--;) {
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 + 70,  1 + 0.6*I);
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 - 70, -1 + 0.6*I);
		WAIT(25);
	}
}

// more bursts. fairies move / \ like
TASK(burst_fairies_2, NO_ARGS) {
	for(int i = 3; i--;) {
		double ofs = 70 + i * 40;
		INVOKE_TASK(burst_fairy, ofs,               1 + 0.6*I);
		WAIT(15);
		INVOKE_TASK(burst_fairy, VIEWPORT_W - ofs, -1 + 0.6*I);
		WAIT(15);
	}
}

TASK(burst_fairies_3, NO_ARGS) {
	for(int i = 10; i--;) {
		cmplx pos = VIEWPORT_W/2 - 200 * sin(1.17 * global.frames);
		INVOKE_TASK(burst_fairy, pos, rng_sign());
		WAIT(60);
	}
}

// swirl, sine pass
TASK(sinepass_swirls, { int duration; double level; double dir; }) {
	int duration = ARGS.duration;
	double dir = ARGS.dir;
	cmplx pos = CMPLX(ARGS.dir < 0 ? VIEWPORT_W : 0, ARGS.level);
	int delay = difficulty_value(30, 20, 15, 10);

	for(int t = 0; t < duration; t += delay) {
		INVOKE_TASK(sinepass_swirl, pos, 3.5 * dir, 7.0 * I);
		WAIT(delay);
	}
}

// big fairies, circle + projectile toss
TASK(circletoss_fairies_1, NO_ARGS) {
	for(int i = 0; i < 2; ++i) {
		INVOKE_TASK(circletoss_fairy,
			.pos = VIEWPORT_W * i + VIEWPORT_H / 3 * I,
			.velocity = 2 - 4 * i - 0.3 * I,
			.exit_accel = 0.03 * (1 - 2 * i) - 0.04 * I ,
			.exit_time = (global.diff > D_Easy) ? 500 : 240
		);

		WAIT(50);
	}
}

TASK(drop_swirls, { int cnt; cmplx pos; cmplx vel; cmplx accel; }) {
	for(int i = 0; i < ARGS.cnt; ++i) {
		INVOKE_TASK(drop_swirl, ARGS.pos, ARGS.vel, ARGS.accel);
		WAIT(20);
	}
}

TASK(schedule_swirls, NO_ARGS) {
	INVOKE_TASK(drop_swirls, 25, VIEWPORT_W/3, 2*I, 0.06);
	WAIT(400);
	INVOKE_TASK(drop_swirls, 25, 200*I, 4, -0.06*I);
}

TASK(circle_fairies_1, NO_ARGS) {
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, VIEWPORT_W - 64, VIEWPORT_W/2 - 100 + 200 * I + 128 * j);
			WAIT(60);
		}

		WAIT(90);

		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, 64, VIEWPORT_W/2 + 100 + 200 * I - 128 * j);
			WAIT(60);
		}

		WAIT(240);
	}
}

TASK(multiburst_fairies_1, NO_ARGS) {
	for(int row = 0; row < 3; ++row) {
		for(int col = 0; col < 5; ++col) {
			cmplx pos = rng_range(0, VIEWPORT_W);
			cmplx target_pos = 64 + 64 * col + I * (64 * row + 100);
			cmplx exit_accel = 0.02 * I + 0.03;
			INVOKE_TASK(multiburst_fairy, pos, target_pos, exit_accel);

			WAIT(10);
		}

		WAIT(120);
	}
}

TASK(instantcircle_fairies, { int duration; }) {
	int interval = difficulty_value(160, 130, 100, 70);

	for(int t = ARGS.duration; t > 0; t -= interval) {
		double x = VIEWPORT_W/2 + 205 * sin(2.13*global.frames);
		double y = VIEWPORT_H/2 + 120 * cos(1.91*global.frames);
		INVOKE_TASK(instantcircle_fairy, x, x+y*I, 0.2 * I);
		WAIT(interval);
	}
}

TASK(waveshot_fairies, { int duration; }) {
	int interval = 200;

	for(int t = ARGS.duration; t > 0; t -= interval) {
		double x = VIEWPORT_W/2 + round(rng_sreal() * 69);
		double y = rng_range(200, 240);
		INVOKE_TASK(waveshot_fairy, x, x+y*I, 0.15 * I);
		WAIT(interval);
	}
}

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();
	boss->move = move_towards(VIEWPORT_W/2.0 + 200.0*I, 0.035);
}

TASK_WITH_INTERFACE(midboss_flee, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();
	boss->move = move_towards(-250 + 30 * I, 0.02);
}

TASK(spawn_midboss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage1_spawn_cirno(VIEWPORT_W + 220 + 30.0*I);

	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Icy Storm", 20, 24000, TASK_INDIRECT(BossAttack, stage1_midboss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage1_spells.mid.perfect_freeze, false);
	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_flee), NULL);

	boss_start_attack(boss, boss->attacks);

	WAIT(60);
	stage1_bg_enable_snow();
}

TASK(tritoss_fairy, { cmplx pos; cmplx velocity; cmplx end_velocity; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 16000, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 5,
		.power = 5,
	});

	e->move = move_linear(ARGS.velocity);
	WAIT(60);
	e->move.retention = 0.9;
	WAIT(20);

	int interval = difficulty_value(12, 9, 5, 3);
	int rounds = 680/interval;
	for(int k = 0; k < rounds; k++) {
		play_sfx("shot1");

		float a = M_PI / 30.0 * ((k/7) % 30) + 0.1 * rng_f32();
		int n = difficulty_value(3,4,4,5);

		for(int i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.move = move_asymptotic_simple(2*cdir(a+2.0*M_PI/n*i), 3),
			);
		}

		if(k == rounds/2 || k == rounds-1) {
			play_sfx("shot_special1");
			int n2 = difficulty_value(20, 23, 26, 30);
			for(int i = 0; i < n2; i++) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos,
					.color = RGB(0.6, 0.2, 0.7),
					.move = move_asymptotic_simple(1.5*cdir(2*M_PI/n2*i),2),
				);

				if(global.diff > D_Easy) {
					PROJECTILE(
						.proto = pp_rice,
						.pos = e->pos,
						.color = RGB(0.6, 0.2, 0.7),
						.move = move_asymptotic_simple(3*cdir(2*M_PI/n2*i), 3.0),
					);
				}
			}
		}
		WAIT(interval);
	}

	WAIT(20);
	e->move = move_asymptotic_simple(ARGS.end_velocity, -1);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = ENT_UNBOX(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK(boss);

	Boss *boss = global.boss = stage1_spawn_cirno(-230 + 100.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage1PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage1PreBossDialog, pm->dialog->Stage1PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage1boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "Iceplosion 0", 20, 23000, TASK_INDIRECT(BossAttack, stage1_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage1_spells.boss.crystal_rain, false);
	boss_add_attack_task(boss, AT_Normal, "Iceplosion 1", 20, 24000, TASK_INDIRECT(BossAttack, stage1_boss_nonspell_2), NULL);

	if(global.diff > D_Normal) {
		boss_add_attack_from_info(boss, &stage1_spells.boss.snow_halation, false);
	}

	boss_add_attack_from_info(boss, &stage1_spells.boss.icicle_cascade, false);
	boss_add_attack_from_info(boss, &stage1_spells.extra.crystal_blizzard, false);

	boss_start_attack(boss, boss->attacks);
}

static void stage1_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage1PostBossDialog, pm->dialog->Stage1PostBoss);
}

DEFINE_EXTERN_TASK(stage1_timeline) {
	INVOKE_TASK_DELAYED(100, burst_fairies_1);
	INVOKE_TASK_DELAYED(240, burst_fairies_2);
	INVOKE_TASK_DELAYED(440, sinepass_swirls, 180, 100, 1);
	INVOKE_TASK_DELAYED(480, circletoss_fairies_1);
	INVOKE_TASK_DELAYED(660, circle_fairies_1);
	INVOKE_TASK_DELAYED(900, schedule_swirls);
	INVOKE_TASK_DELAYED(1500, burst_fairies_3);
	INVOKE_TASK_DELAYED(2200, multiburst_fairies_1);

	INVOKE_TASK_DELAYED(2200, common_call_func, stage1_bg_raise_camera);
	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);
	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	int midboss_time = WAIT_EVENT(&global.boss->events.defeated).frames;
	int filler_time = 2180;
	int time_ofs = 500 - midboss_time;

	log_debug("midboss_time = %i; filler_time = %i; time_ofs = %i", midboss_time, filler_time, time_ofs);

	STAGE_BOOKMARK(post-midboss);

	int swirl_spam_time = 760;

	for(int i = 0; i < swirl_spam_time; i += 30) {
		int o = ((int[]) { 0, 1, 0, -1 })[(i / 60) % 4];
		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 40, 132 + 32 * o, 1 - 2 * ((i / 60) & 1));
	}

	time_ofs += swirl_spam_time;

	INVOKE_TASK_DELAYED(40 + time_ofs, burst_fairies_1);

	int instacircle_time = filler_time - swirl_spam_time - 600;

	for(int i = 0; i < instacircle_time; i += 180) {
		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 80, 132, 1);
		INVOKE_TASK_DELAYED(120 + i + time_ofs, instantcircle_fairies, 120);
	}

	WAIT(filler_time - midboss_time);
	STAGE_BOOKMARK(post-midboss-filler);

	INVOKE_TASK_DELAYED(100, circletoss_fairy,           -25 + VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
	INVOKE_TASK_DELAYED(125, circletoss_fairy, VIEWPORT_W+25 + VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(115, circletoss_fairy,           -25 + 2*VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
		INVOKE_TASK_DELAYED(140, circletoss_fairy, VIEWPORT_W+25 + 2*VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);
	}

	STAGE_BOOKMARK_DELAYED(200, waveshot-fairies);

	INVOKE_TASK_DELAYED(240, waveshot_fairies, 600);
	INVOKE_TASK_DELAYED(400, burst_fairies_3);

	STAGE_BOOKMARK_DELAYED(1000, post-midboss-filler-2);

	INVOKE_TASK_DELAYED(1000, burst_fairies_1);
	INVOKE_TASK_DELAYED(1120, explosion_fairy,              120*I, VIEWPORT_W-80 + 120*I, -0.2+0.1*I);
	INVOKE_TASK_DELAYED(1280, explosion_fairy, VIEWPORT_W + 220*I,            80 + 220*I,  0.2+0.1*I);

	STAGE_BOOKMARK_DELAYED(1400, post-midboss-filler-3);

	INVOKE_TASK_DELAYED(1400, drop_swirls, 25, 2*VIEWPORT_W/3, 2*I, -0.06);
	INVOKE_TASK_DELAYED(1600, drop_swirls, 25,   VIEWPORT_W/3, 2*I,  0.06);

	INVOKE_TASK_DELAYED(1520, tritoss_fairy, VIEWPORT_W / 2 - 30*I, 3 * I, -2.6 * I);

	INVOKE_TASK_DELAYED(1820, circle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W - 130 + 240*I);
	INVOKE_TASK_DELAYED(1820, circle_fairy,            - 42 + 300*I,              130 + 240*I);

	INVOKE_TASK_DELAYED(1880, instantcircle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W -  84 + 260*I, 0.2 * (-2 - I));
	INVOKE_TASK_DELAYED(1880, instantcircle_fairy,            - 42 + 300*I,               84 + 260*I, 0.2 * ( 2 - I));

	INVOKE_TASK_DELAYED(2120, waveshot_fairy, VIEWPORT_W + 42 + 300*I,               130 + 140*I, 0.2 * (-2 - I));
	INVOKE_TASK_DELAYED(2120, waveshot_fairy,            - 42 + 300*I, VIEWPORT_W -  130 + 140*I, 0.2 * ( 2 - I));

	STAGE_BOOKMARK_DELAYED(2300, pre-boss);

	WAIT(2560);
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stage1boss");

	WAIT(120);
	stage1_bg_disable_snow();
	WAIT(120);

	stage1_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
