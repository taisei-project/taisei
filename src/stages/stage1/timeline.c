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
#include "enemy_classes.h"

TASK(burst_fairy, { BoxedEnemy e; cmplx target_pos; cmplx exit_dir; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = move_from_towards(e->pos, ARGS.target_pos, 0.03);

	WAIT(difficulty_value(120, 80, 60, 60));

	play_sfx("shot1");
	int n = difficulty_value(0, 1, 3, 5);

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
	e->move.acceleration = 0.04 * ARGS.exit_dir;
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

	real spread = difficulty_value(0.05, 0.1, 0.15, 0.2);

	while(ARGS.times--) {
		for(int i = ARGS.duration; i--;) {
			play_sfx_loop("shot1_loop");

			real aim_angle = carg(global.plr.pos - e->pos) + spread * 0.5 * rng_sreal();
			cmplx aim = cdir(aim_angle) * rng_range(1, 3);

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
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1)));
	e->move = move_linear(ARGS.velocity);

	ARGS.exit_time -= WAIT(difficulty_value(80, 80, 60, 60));

	INVOKE_SUBTASK(circletoss_shoot_circle, ENT_BOX(e),
		.duration = 40,
		.interval = difficulty_value(3, 3, 2, 2)
	);

	if(global.diff > D_Easy) {
		INVOKE_SUBTASK_DELAYED(30, circletoss_shoot_toss, ENT_BOX(e),
			.times = 4,
			.period = 150,
			.duration = difficulty_value(-1, 15, 26, 33)
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
		sv -= im(e->pos - e->pos0) * 0.03 * I;
		e->pos += sv * 0.4 + v;
		YIELD;
	}
}

TASK(sinepass_swirl, { cmplx pos; cmplx vel; cmplx svel; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 1)));
	INVOKE_TASK(sinepass_swirl_move, ENT_BOX(e), ARGS.vel, ARGS.svel);

	WAIT(difficulty_value(40, 40, 15, 10));

	int shot_interval = difficulty_value(155, 80, 30, 20);

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
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 2)));

	e->move = move_from_towards(e->pos, ARGS.target_pos, 0.005);
	e->move.retention = 0.8;

	WAIT(60);

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
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 2)));
	e->move = move_accelerated(ARGS.vel, ARGS.accel);

	int shot_interval = difficulty_value(120, 120, 30, 20);

	WAIT(difficulty_value(50, 35, 20, 20));

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

TASK(multiburst_fairy, { BoxedEnemy e; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = move_from_towards(e->pos, ARGS.target_pos, 0.05);

	WAIT(difficulty_value(120, 60, 60, 60));

	int burst_interval = difficulty_value(40, 30, 18, 16);
	int bursts = 4;

	for(int i = 0; i < bursts; ++i) {
		play_sfx("shot1");
		int n = difficulty_value(0, 0, 2, 3);

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
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));
	e->move = move_from_towards(e->pos, ARGS.target_pos, 0.04);
	BoxedEnemy be = ENT_BOX(e);

	INVOKE_TASK_DELAYED(75, instantcircle_fairy_shoot, be,
		.cnt = difficulty_value(20, 22, 24, 28),
		.speed = 1.5,
		.boost = 2.0
	);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(95, instantcircle_fairy_shoot, be,
			.cnt = difficulty_value(0, 26, 29, 32),
			.speed = difficulty_value(0, 2.0, 3.0, 3.0),
			.boost = 3.0
		);
	}

	WAIT(200);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(waveshot, { cmplx pos; cmplx dir; real spread; real freq; int shots; int interval; } ) {
	for(int i = 0; i < ARGS.shots; ++i) {
		cmplx v = 4 * ARGS.dir * cdir(ARGS.spread * triangle(ARGS.freq * i));

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

TASK(waveshot_fairy, { cmplx pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));
	ecls_anyfairy_summon(e, 120);

	cmplx dir = cnormalize(global.plr.pos - e->pos);
	cmplx ofs = -24 * dir;

	common_charge(60, &e->pos, ofs, RGBA(0.0, 0.25, 0.5, 0));

	real spread = difficulty_value(M_PI/20, M_PI/18, M_PI/16, M_PI/14);
	real interval = difficulty_value(3, 2, 1, 1);
	real shots = 60 / interval;
	real frequency = 60 * (1.0/12.0) / shots;
	shots += 1;

	INVOKE_SUBTASK(waveshot, e->pos + ofs, dir, rng_sign() * spread, frequency, shots, interval);

	WAIT(120);

	e->move.attraction = 0;
	e->move.retention = 0.8;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(explosion_fairy, { cmplx pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 8)));
	INVOKE_SUBTASK_DELAYED(80, common_charge, {
		.time = 120,
		.pos = e->pos,
		.color = RGBA(1.0, 0, 0.2, 0),
		.sound = COMMON_CHARGE_SOUNDS,
	});
	ecls_anyfairy_summon(e, 120);

	WAIT(80);

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
TASK(burst_fairies_1) {
	for(int i = 3; i--;) {
		INVOKE_TASK(burst_fairy,
			.e = espawn_fairy_blue_box(VIEWPORT_W/2 + 70, ITEMS(.points = 1)),
			.target_pos = VIEWPORT_W/2 + 70 + (120 + 42 * i)*I,
			.exit_dir = 1 + 0.6*I
		);
		INVOKE_TASK(burst_fairy,
			.e = espawn_fairy_red_box(VIEWPORT_W/2 - 70, ITEMS(.power = 1)),
			.target_pos = VIEWPORT_W/2 - 70 + (120 + 42 * i)*I,
			.exit_dir = -1 + 0.6*I
		);
		WAIT(25);
	}
}

// more bursts. fairies move \ / like
TASK(burst_fairies_2) {
	for(int i = 3; i--;) {
		real ofs = 70 + i * 40;

		EnemySpawner s0, s1;
		ItemCounts i0, i1;

		if(i & 1) {
			s0 = espawn_fairy_red;
			i0 = *ITEMS(.power = 1);
			s1 = espawn_fairy_blue;
			i1 = *ITEMS(.points = 1);
		} else {
			s0 = espawn_fairy_blue;
			i0 = *ITEMS(.points = 1);
			s1 = espawn_fairy_red;
			i1 = *ITEMS(.power = 1);
		}

		cmplx p0 = ofs;
		cmplx p1 = VIEWPORT_W - ofs;

		INVOKE_TASK(burst_fairy, ENT_BOX(s0(p0, &i0)), p0 + 120*I, 0.6*I + 1);
		WAIT(15);
		INVOKE_TASK(burst_fairy, ENT_BOX(s1(p1, &i1)), p1 + 120*I, 0.6*I - 1);
		WAIT(15);
	}
}

TASK(burst_fairies_3) {
	for(int i = 10; i--;) {
		cmplx pos = VIEWPORT_W/2 - 200 * sin(1.17 * global.frames);
		INVOKE_TASK(burst_fairy,
			(i & 1)
				? espawn_fairy_blue_box(pos, ITEMS(.points = 1))
				: espawn_fairy_red_box(pos, ITEMS(.power = 1)),
			pos + 120*I, rng_sign()
		);
		WAIT(60);
	}
}

// swirl, sine pass
TASK(sinepass_swirls, { int duration; real level; real dir; }) {
	int duration = ARGS.duration;
	real dir = ARGS.dir;
	cmplx pos = CMPLX(ARGS.dir < 0 ? VIEWPORT_W : 0, ARGS.level);
	int delay = difficulty_value(30, 30, 15, 10);

	for(int t = 0; t < duration; t += delay) {
		INVOKE_TASK(sinepass_swirl, pos, 3.5 * dir, 7.0 * I);
		WAIT(delay);
	}
}

// big fairies, circle + projectile toss
TASK(circletoss_fairies_1) {
	for(int i = 0; i < 2; ++i) {
		INVOKE_TASK(circletoss_fairy,
			.pos = VIEWPORT_W * i + VIEWPORT_H / 3 * I,
			.velocity = 2 - 4 * i - 0.3 * I,
			.exit_accel = 0.03 * (1 - 2 * i) - 0.04 * I ,
			.exit_time = (global.diff > D_Easy) ? 500 : 240
		);

		WAIT(250);
	}
}

TASK(drop_swirls, { int cnt; cmplx pos; cmplx vel; cmplx accel; }) {
	for(int i = 0; i < ARGS.cnt; ++i) {
		INVOKE_TASK(drop_swirl, ARGS.pos, ARGS.vel, ARGS.accel);
		WAIT(20);
	}
}

TASK(schedule_swirls) {
	INVOKE_TASK(drop_swirls, 25, VIEWPORT_W/3, 2*I, 0.06);
	WAIT(400);
	INVOKE_TASK(drop_swirls, 25, 200*I, 4, -0.06*I);
}

TASK(circle_fairies_1) {
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

TASK(multiburst_fairies_1) {
	int rows = difficulty_value(1, 2, 3, 3);
	int row_interval = difficulty_value(0, 160, 120, 120);
	int cols = 5;
	int col_interval = difficulty_value(30, 10, 10, 10);

	for(int row = 0; row < rows; ++row) {
		for(int col = 0; col < cols; ++col) {
			cmplx pos = rng_range(0, VIEWPORT_W);
			cmplx target_pos = 64 + 64 * col + I * (64 * row + 100);
			cmplx exit_accel = 0.02 * I + 0.03;
			Enemy *e;

			if(row & 1) {
				e = espawn_fairy_red(pos, ITEMS(.power = 2));
			} else {
				e = espawn_fairy_blue(pos, ITEMS(.points = 3));
			}

			INVOKE_TASK(multiburst_fairy, ENT_BOX(e), target_pos, exit_accel);

			WAIT(col_interval);
		}

		WAIT(row_interval);
	}
}

TASK(instantcircle_fairies, { int duration; int seq_ofs; }) {
	int interval = difficulty_value(160, 130, 100, 70);

	real xofs[] = {  0.32, -0.85, 0.93, -0.42, -0.9 };
	real yofs[] = { -0.69, -0.93, 0.80,  0.21, -0.96, -0.53, 0.95 };

	for(int t = ARGS.duration, i = ARGS.seq_ofs; t > 0; t -= interval, ++i) {
		log_debug("i = %i; x=%f, y=%f", i, xofs[i % ARRAY_SIZE(xofs)], yofs[i % ARRAY_SIZE(yofs)]);

		real x = VIEWPORT_W/2 + 205 * xofs[i % ARRAY_SIZE(xofs)];
		real y = VIEWPORT_H/2 + 100 * yofs[i % ARRAY_SIZE(yofs)];
		INVOKE_TASK(instantcircle_fairy, x, x+y*I, 0.2 * I);
		WAIT(interval);
	}
}

TASK(waveshot_fairies, { int duration; }) {
	int interval = 200;

	for(int t = ARGS.duration; t > 0; t -= interval) {
		real x = VIEWPORT_W/2 + round(rng_sreal() * 69);
		real y = rng_range(200, 240);
		INVOKE_TASK(waveshot_fairy, x+y*I, 0.15 * I);
		WAIT(interval);
	}
}

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 200.0*I, 0.035);
}

TASK_WITH_INTERFACE(midboss_flee, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, -250 + 30 * I, 0.02);
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage1_spawn_cirno(VIEWPORT_W + 220 + 30.0*I);

	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Icy Storm", 20, 24000, TASK_INDIRECT(BossAttack, stage1_midboss_nonspell_1), NULL);
	if(global.diff > D_Normal) {
		boss_add_attack_from_info(boss, &stage1_spells.mid.perfect_freeze, false);
	}
	boss_add_attack_task(boss, AT_Move, "Flee", 2, 0, TASK_INDIRECT(BossAttack, midboss_flee), NULL);

	boss_engage(boss);

	WAIT(60);
	stage1_bg_enable_snow();
}

TASK(tritoss_fairy, { cmplx pos; cmplx end_velocity; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(.points = 5, .power = 6)));
	INVOKE_SUBTASK_DELAYED(120, common_charge, {
		.pos = e->pos,
		.time = 60,
		.color = RGBA(0.1, 0.2, 1.0, 0),
		.sound = COMMON_CHARGE_SOUNDS,
	});
	ecls_anyfairy_summon(e, 180);

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

	WAIT(60);
	e->move = move_asymptotic_simple(ARGS.end_velocity, -1);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK(boss);
	stage_unlock_bgm("stage1");

	Boss *boss = global.boss = stage1_spawn_cirno(-230 + 100.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage1PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage1PreBossDialog, pm->dialog->Stage1PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage1boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	STAGE_BOOKMARK(boss-postdialog);

	boss_add_attack_task(boss, AT_Normal, "Iceplosion 0", 20, 26000, TASK_INDIRECT(BossAttack, stage1_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage1_spells.boss.crystal_rain, false);
	boss_add_attack_task(boss, AT_Normal, "Iceplosion 1", 20, 30000, TASK_INDIRECT(BossAttack, stage1_boss_nonspell_2), NULL);

	if(global.diff > D_Normal) {
		boss_add_attack_from_info(boss, &stage1_spells.boss.snow_halation, false);
	}

	boss_add_attack_from_info(boss, &stage1_spells.boss.icicle_cascade, false);
	boss_add_attack_from_info(boss, &stage1_spells.extra.crystal_blizzard, false);

	boss_engage(boss);
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
	int time_ofs = -midboss_time;

	log_debug("midboss_time = %i; filler_time = %i; time_ofs = %i", midboss_time, filler_time, time_ofs);

	STAGE_BOOKMARK(post-midboss);

	int swirl_spam_time = 1260;

	for(int i = 0; i < /*swirl_spam_time*/ filler_time; i += 30) {
		int o = ((int[]) { 0, 1, 0, -1 })[(i / 60) % 4];
		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 40, 132 + 32 * o, 1 - 2 * ((i / 60) & 1));
	}

	time_ofs += swirl_spam_time;

	INVOKE_TASK_DELAYED(time_ofs, burst_fairies_1);

	int instacircle_time = filler_time - swirl_spam_time - 100;

	for(int t = 0, i = 1; t < instacircle_time; t += 180, ++i) {
// 		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 80, 132, 1);
		INVOKE_TASK_DELAYED(120 + t + time_ofs, instantcircle_fairies, 120, i);
	}

	WAIT(filler_time - midboss_time);
	STAGE_BOOKMARK(post-midboss-filler);

	INVOKE_TASK_DELAYED(100, circletoss_fairy,           -25 + VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
	INVOKE_TASK_DELAYED(125, circletoss_fairy, VIEWPORT_W+25 + VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(115, circletoss_fairy,           -25 + 2*VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
		INVOKE_TASK_DELAYED(140, circletoss_fairy, VIEWPORT_W+25 + 2*VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);
	}

	STAGE_BOOKMARK_DELAYED(180, waveshot-fairies);

	INVOKE_TASK_DELAYED(180, waveshot_fairies, 600);
	INVOKE_TASK_DELAYED(400, burst_fairies_3);
	INVOKE_TASK_DELAYED(430, burst_fairies_3);

	STAGE_BOOKMARK_DELAYED(1000, post-midboss-filler-2);

	INVOKE_TASK_DELAYED(1000, burst_fairies_1);
	INVOKE_TASK_DELAYED(1000, explosion_fairy, VIEWPORT_W-80 + 120*I, -0.2+0.1*I);
	INVOKE_TASK_DELAYED(1140, explosion_fairy,            80 + 220*I,  0.2+0.1*I);

	STAGE_BOOKMARK_DELAYED(1400, post-midboss-filler-3);

	INVOKE_TASK_DELAYED(1400, drop_swirls, 25, 2*VIEWPORT_W/3, 2*I, -0.06);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(1600, drop_swirls, 25,   VIEWPORT_W/3, 2*I,  0.06);
	}

	INVOKE_TASK_DELAYED(1420, tritoss_fairy, VIEWPORT_W / 2 + 220*I, -2.6 * I);

	INVOKE_TASK_DELAYED(1820, circle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W - 130 + 240*I);
	INVOKE_TASK_DELAYED(1820, circle_fairy,            - 42 + 300*I,              130 + 240*I);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(1880, instantcircle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W -  84 + 260*I, 0.2 * (-2 - I));
		INVOKE_TASK_DELAYED(1880, instantcircle_fairy,            - 42 + 300*I,               84 + 260*I, 0.2 * ( 2 - I));
	}

	INVOKE_TASK_DELAYED(2060, waveshot_fairy,               130 + 140*I, 0.2 * (-2 - I));
	INVOKE_TASK_DELAYED(2060, waveshot_fairy, VIEWPORT_W -  130 + 140*I, 0.2 * ( 2 - I));

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
