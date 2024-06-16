/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

// Pattern contributed by raz, originally written for danmakufu
// The port is almost direct, and a bit rough

TASK(TColumn, { BoxedBoss boss; cmplx offset; cmplx aim; int num; real rank; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	// NOTE: bullet flash warning dummied out, replaced with simple wait
	WAIT(20);

	int num = ARGS.num;
	real rank = ARGS.rank;
	cmplx shot_origin = boss->pos + ARGS.offset;

	for(int i = 0; i < num; ++i) {
		real spd = rank * (3.75 + (1.875 * i) / num);
		cmplx aim = ARGS.aim * cdir(M_PI/180 * rng_sreal());

		play_sfx_loop("shot1_loop");

		PROJECTILE(
			.proto = pp_card,
			.color = RGB(1, 0, 0),
			.move = move_asymptotic_simple(spd * aim, 2),
			.pos = shot_origin,
		);

		WAIT(5);
	}
}

TASK(XPattern, { BoxedBoss boss; real dir_sign; int num2; int num3; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real rank = difficulty_value(0.4, 0.65, 0.85, 1);
	int rof = lround(3 / rank);
	int num = 180 * rank;
	int num2 = ARGS.num2 * rank;
	int num3 = ARGS.num3 * rank;
	real odd = 1;
	real dir_sign = ARGS.dir_sign;
	cmplx rot = cnormalize(global.plr.pos - boss->pos) * cdir(M_PI/2 * -dir_sign);

	for(int i = 0; i < num2; ++i) {
		for(int j = 0; j < 3; ++j) {
			cmplx dir = rot * cdir(dir_sign * (i * M_TAU*2 / (num - 1.0) - odd * (M_PI/6 + M_PI/3 * j)));

			INVOKE_SUBTASK(TColumn,
				.boss = ENT_BOX(boss),
				.offset = 80 * dir,
				.aim = dir * cdir(M_PI/4 * dir_sign * odd),
				.num = num3,
				.rank = rank
			);
		}

		odd = -odd;
		WAIT(rof);
	}
}

TASK(TShoot, { BoxedBoss boss; real ang; real ang_s; real dist; real spd_inc; real dir_sign; int t; int time; int num; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real ang = ARGS.ang;
	real ang_s = ARGS.ang_s;
	real dist = ARGS.dist;
	real spd_inc = ARGS.spd_inc;
	real dir_sign = ARGS.dir_sign;
	int t = ARGS.t;
	int time = ARGS.time;
	int num = ARGS.num;

	for(int i = 0; i < num; ++i) {
		real ang2 = ang - ang_s * i * dir_sign;
		cmplx shot_origin = boss->pos + cdir(ang2) * dist;
		real spd = (1.875 - 0.625 * (i / num)) + (spd_inc * t) / time;
		cmplx dir = cdir(ang2 - M_PI/2 * dir_sign);

		bool phase2 = t > 0.6 * time;

		play_sfx(phase2 ? "shot1" : "shot2");

		PROJECTILE(
			.proto = phase2 ? pp_crystal : pp_card,
			.color = RGB(t / (real)time, 0, 1),
			.pos = shot_origin,
			.move = move_linear(spd * dir),
		);

		WAIT(3);
	}
}

TASK(SpinPattern, { BoxedBoss boss; real dir_sign; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int rof = difficulty_value(12, 5, 4, 3);
	real spd_inc = difficulty_value(1.25, 1.875, 2.5, 3.125);

	int time = 240;
	int num = 3;
	real dir_sign = ARGS.dir_sign;
	real ang = M_PI/2 * (1 - dir_sign);
	real ang_d = dir_sign * rof * (2*M_TAU - M_PI/2) / time;
	real ang_s = M_TAU/3;
	real ang_s_d = -(M_TAU/3) / (time / (real)rof);
	real dist = 20;
	real dist_d = 320.0 / time;

	// NOTE: bullet flash warning dummied out, replaced with simple wait

	WAIT(20);

	for(int t = 0; t < time;) {
		INVOKE_SUBTASK(TShoot,
			.boss = ENT_BOX(boss),
			.ang = ang,
			.ang_s = ang_s,
			.dist = dist,
			.spd_inc = spd_inc,
			.dir_sign = dir_sign,
			.t = t,
			.time = time,
			.num = num
		);

		ang_s += ang_s_d;
		dist += dist_d;
		ang += ang_d;

		t += WAIT(rof);
	}
}

static cmplx random_boss_pos(Boss *boss) {
	Rect bounds = viewport_bounds(140);
	bounds.top = 140;
	bounds.bottom = 200;
	return common_wander(boss->pos, 60, bounds);
	// return VIEWPORT_W/2 + (rng_sreal() * 40 + I * rng_range(165, 180));
}

static void random_move(Boss *boss) {
	boss->move.attraction_point = random_boss_pos(boss);
	aniplayer_soft_switch(&boss->ani, "guruguru", 1);
	aniplayer_queue(&boss->ani, "main", 0);
}

DEFINE_EXTERN_TASK(stage2_boss_nonspell_3) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	real dir_sign = rng_sign();

	random_move(boss);

	WAIT(20);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(30);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(30);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(210);
	INVOKE_SUBTASK(XPattern,
		.boss = ENT_BOX(boss),
		.dir_sign = dir_sign,
		.num2 = 42,
		.num3 = 6
	);

	for(;;) {
		dir_sign = -dir_sign;

		random_move(boss);

		WAIT(80);

		INVOKE_SUBTASK(XPattern,
			.boss = ENT_BOX(boss),
			.dir_sign = dir_sign,
			.num2 = 42,
			.num3 = 6
		);

		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(30);
		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(30);
		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(180);

		INVOKE_SUBTASK(XPattern,
			.boss = ENT_BOX(boss),
			.dir_sign = dir_sign,
			.num2 = 42,
			.num3 = 9
		);
	}
}
