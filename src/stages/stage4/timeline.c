/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"

#include "background_anim.h"
#include "kurumi.h"
#include "nonspells/nonspells.h"
#include "stage4.h"

#include "stages/common_imports.h"
#include "../stage6/elly.h"

static void stage4_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage4PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage4PreBossDialog, pm->dialog->Stage4PreBoss, &e);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage4boss");
}

static void stage4_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage4PostBossDialog, pm->dialog->Stage4PostBoss);
}

TASK(filler_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e;
	if(rng_chance(0.5)) {
		e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 1)));
	} else {
		e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	}

	e->move = ARGS.move;
}

TASK(splasher_fairy, { cmplx pos; int direction; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1, .bomb = 1)));

	cmplx move = 3 * ARGS.direction - 4.0 * I;
	e->move = move_linear(move);
	WAIT(25);
	e->move = move_dampen(e->move.velocity, 0.85);

	WAIT(50);
	int duration = difficulty_value(150, 160, 180, 210);
	int interval = difficulty_value(20, 15, 10, 5);
	for(int time = 0; time < duration; time += WAIT(interval)) {
		RNG_ARRAY(rand, 4);
		cmplx offset = 50 * vrng_real(rand[0]) * cdir(vrng_angle(rand[1]));
		cmplx v = (0.5 + vrng_real(rand[2])) * cdir(-0.1 * time * re(ARGS.direction))*I;

		int petals = 5;
		if(rng_chance(0.5)) {
			for(int i = 0; i < petals; i++) {
				cmplx petal_pos = 15 * cdir(M_TAU/petals * i + vrng_angle(rand[3]));

					PROJECTILE(
						.proto = pp_wave,
						.pos = e->pos + offset + petal_pos,
						.color = RGBA(1.0, 0.0, 0.0, 0.0),
						.move = move_accelerated(v - 0.1 * I, 0.01 * I),
						.angle = carg(petal_pos),
						.flags = PFLAG_MANUALANGLE
					);
			}
		} else {
			for(int i = 0; i < petals; i++) {
				cmplx petal_pos = 5 * cdir(M_TAU/petals * i + vrng_angle(rand[3]));
				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos + offset + petal_pos,
					.color = RGBA(1.0, 0.0, 0.5, 0.0),
					.move = move_accelerated(v - 0.1 * I, 0.01 * v + 0.001 * cnormalize(petal_pos)),
					.angle = carg(petal_pos),
					.flags = PFLAG_MANUALANGLE
				);
			}
		}
		play_sfx_loop("shot1_loop");
	}

	WAIT(60);
	e->move = move_linear(-ARGS.direction);
}

TASK(fodder_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	e->move = ARGS.move;

	WAIT(10);
	for(int i = 0; i < 2; i++, WAIT(120)) {
		cmplx fairy_halfsize = 21 * (1 + I);

		if(!rect_rect_intersect(
			(Rect) { e->pos - fairy_halfsize, e->pos + fairy_halfsize },
			(Rect) { 0, CMPLX(VIEWPORT_W, VIEWPORT_H) },
			true, true)
		) {
			continue;
		}

		play_sfx_ex("shot3", 5, false);
		cmplx aim = cnormalize(global.plr.pos - e->pos);

		real speed = difficulty_value(2, 2.3, 2.6, 3);
		real boost_factor = 1.2;
		real boost_base = 1;
		int chain_len = difficulty_value(3, 3, 5, 5);
		real fire_chance = difficulty_value(0.2,0.45,0.75,1.0);

		if(global.diff > D_Easy) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(1.0, 0.0, 0.5),
				.move = move_linear(rng_dir()),
			);
		}

		if(rng_chance(fire_chance)) {
			for(int j = 0; j < chain_len; ++j) {
				PROJECTILE(
					.proto = j & 1 ? pp_rice : pp_wave,
					.pos = e->pos,
					.color = RGB(j / (float)(chain_len+1), 0.0, 0.5),
					.move = move_asymptotic_simple(speed * aim, boost_base + j * boost_factor),
					.max_viewport_dist = 32,
				);
			}
		}
	}
}

TASK(partcircle_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 3)));

	e->move = ARGS.move;

	// TODO: replace me by the new common task
	WAIT(30);
	e->move.retention = 0.9;
	WAIT(30);
	e->move.retention = 1;

	int circles = difficulty_value(1,2,3,4);
	int projs_per_circle = difficulty_value(20,30,35,40);

	for(int i = 0; i < projs_per_circle; i++) {
		for(int j = 0; j < circles; j++) {
			play_sfx("shot2");

			real angle_offset = carg(global.plr.pos - e->pos);
			cmplx dir = cdir((i - 4) * M_TAU / projs_per_circle + angle_offset) * cnormalize(ARGS.move.velocity);

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 30 * dir,
				.color = RGB(1 - 0.2 * j, 0.0, 1),
				.move = move_asymptotic_simple(2 * dir, 2 + 0.22 * i + 2 * j),
			);
		}
		YIELD;
	}

	WAIT(90);
	e->move = move_accelerated(0, 0.05 * I);
	WAIT(40);
	e->move.acceleration = 0;
}

TASK(cardbuster_fairy_move, { Enemy *e; int stops; cmplx *poss; int *move_times; int *pause_times; }) {
	for(int i = 0; i < ARGS.stops-1; i++) {
		WAIT(ARGS.pause_times[i]);
		ARGS.e->move = move_linear((ARGS.poss[i+1] - ARGS.e->pos) / ARGS.move_times[i]);
		WAIT(ARGS.move_times[i]);
		ARGS.e->move = move_dampen(ARGS.e->move.velocity, 0.8);
	}
}

TASK(cardbuster_fairy_second_attack, { Enemy *e; }) {
	int count = difficulty_value(35, 55, 75, 90);
	real speed = difficulty_value(1.0, 1.2, 1.4, 1.6);

	INVOKE_SUBTASK(common_charge, 0, *RGBA(0.0, 0.7, 1.0, 0), 40, .anchor = &ARGS.e->pos, .sound = COMMON_CHARGE_SOUNDS);
	WAIT(40);

	for(int i = 0; i < count; i++) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cnormalize(global.plr.pos - ARGS.e->pos) * cdir(2 * M_TAU / (count + 1) * i);
		cmplx v = speed * dir;
		PROJECTILE(
			.proto = pp_card,
			.pos = ARGS.e->pos + 30 * dir,
			.color = RGB(0, 0.7, 0.5),
			.move = move_asymptotic(2*v*I, v, 0.99),
		);
		PROJECTILE(
			.proto = pp_card,
			.pos = ARGS.e->pos + 30 * dir + 4*v,
			.color = RGB(0, 0.2, 1.0),
			.move = move_asymptotic(2*v*I, v, 0.99),
		);
		YIELD;
	}
}

TASK(cardbuster_fairy, { cmplx poss[4]; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.poss[0], ITEMS(.points = 3, .power = 1)));

	int move_times[ARRAY_SIZE(ARGS.poss)-1] = {
		120, 100, 200
	};
	int pause_times[ARRAY_SIZE(ARGS.poss)-1] = {
		0, 80, 100
	};
	int second_attack_delay = 240;

	INVOKE_SUBTASK(cardbuster_fairy_move, e, ARRAY_SIZE(ARGS.poss), ARGS.poss, move_times, pause_times);
	INVOKE_SUBTASK_DELAYED(second_attack_delay, cardbuster_fairy_second_attack, e);

	int count = difficulty_value(40, 80, 120, 160);
	real shotspeed = difficulty_value(1.0, 1.2, 1.4, 1.6);

	WAIT(60);
	for(int i = 0; i < count; i++) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cnormalize(global.plr.pos - e->pos) * cdir(2 * M_TAU / (count + 1) * i);

		for(int j = 0; j < 3; j++) {
			real speed = shotspeed * (1.0 + 0.1 * j);
			real speedup = 0.01 * (1.0 + 0.01 * i);
			PROJECTILE(
				.proto = pp_card,
				.pos = e->pos + 30 * dir,
				.color = RGB(0.5, 0.1, 0.2 + 0.2 * j),
				.move = move_asymptotic_simple(speed * dir, speedup),
			);
		}

		YIELD;
	}

	STALL;
}

TASK(play_laser_sfx) {
	play_sfx("laser1");
}

TASK(laser_fairy_explosion, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	int laser_delay = difficulty_value(200, 180, 150, 120);
	INVOKE_TASK_DELAYED(laser_delay, play_laser_sfx);


	cmplx aim = cnormalize(global.plr.pos - e->pos);
	int count = difficulty_value(4,5,6,8);
	if(global.diff > D_Easy) {
		for(int i = 0; i < count; i++) {
			cmplx dir = aim * cdir(M_TAU * i / count);
			create_laserline_ab(e->pos, e->pos + 1000*dir, 15, laser_delay, laser_delay + 60, RGBA(0.7, 1.0, 0.2, 0.0));
		}
	}

	int count2 = difficulty_value(2, 3, 4, 6) * count;
	play_sfx("shot3");
	for(int i = 0; i < count2; i++) {
		cmplx dir = aim * cdir(M_TAU * i / count2);
		real radius = 2.0 - 4*re(cpow(dir, count));
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGBA(1.0, 0.9, 0.3, 0.0),
			.move = move_asymptotic_simple(1 * dir, radius),
		);
	}
}

TASK(laser_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 5)));
	e->move = ARGS.move;

	INVOKE_TASK_WHEN(&e->events.killed, laser_fairy_explosion, ENT_BOX(e));

	WAIT(20);
	int count = difficulty_value(60, 80, 100, 120);
	real vel = difficulty_value(2.0, 2.3, 2.6, 3.0);
	int interval = difficulty_value(3, 3, 2, 2);
	for(int i = 0; i < count; i++) {
		play_sfx("shot3");
		cmplx dir = cdir((0.1 * i - (1-2*(i&1))*M_PI / 2.0) * sign(re(ARGS.move.velocity)));
		for(int j = 0; j < global.diff; j++) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(1.0, 0.7, 0.3),
				.move = move_asymptotic_simple(vel * dir, 2 + 2 * j),
			);
		}
		WAIT(interval);
	}

	STALL;
}

TASK(bigcircle_fairy_move, { BoxedEnemy enemy; cmplx vel; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	e->move = move_linear(ARGS.vel);
	WAIT(70);
	e->move = move_dampen(e->move.velocity, 0.9);
	WAIT(130);
	e->move = move_linear(-ARGS.vel);
	WAIT(100);
	e->move = move_dampen(e->move.velocity, 0.9);
}

TASK(bigcircle_fairy, { cmplx pos; cmplx vel; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1)));
	INVOKE_TASK(bigcircle_fairy_move, ENT_BOX(e), ARGS.vel);
	WAIT(20);
	INVOKE_SUBTASK(common_charge, 0, *RGBA(0, 0.8, 0, 0), 60, .anchor = &e->pos, .sound = COMMON_CHARGE_SOUNDS);

	WAIT(40);
	int shots = difficulty_value(3, 4, 6, 7);
	for(int i = 0; i < shots; i++) {

		play_sfx("shot_special1");
		int count = difficulty_value(13, 16, 19, 22);
		real ball_speed = difficulty_value(1.7, 1.9, 2.1, 2.3);

		for(int j = 0; j < count; j++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos,
				.color = RGBA(0, 0.8 - 0.4 * i, 0, 0),
				.move = move_asymptotic_simple(2*cdir(M_TAU / count * j + 3 * i), 3 * sin(6 * M_PI / count * j))
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(0, 0.3 * i, 0.4, 0),
					.move = move_asymptotic_simple(ball_speed*cdir(3*(j+rng_real())),
						5 * sin(6 * M_PI / count * j))
				);
			}
		}

		WAIT(30/(1+i/2));
	}
}

TASK(explosive_swirl_explosion, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	int count = difficulty_value(5, 10, 15, 20);
	cmplx aim = cnormalize(global.plr.pos - e->pos);

	real speed = difficulty_value(1.4, 1.7, 2.0, 2.3);

	cmplx dirofs = rng_dir();

	for(int i = 0; i < count; i++) {
		cmplx dir = dirofs * cdir(M_TAU * i / count) * aim;

		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.2, 0.2 + 0.6 * (i&1), 1 - 0.6 * (i&1)),
			.move = move_accelerated(0.5 * speed * dir, 0.001*dir),
		);

		if(global.diff > D_Easy) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.2, 0.2 + 0.6 * (i&1), 1 - 0.6 * (i&1)),
				.move = move_asymptotic_simple(1.5 * speed * dir, 8),
			);
		}
	}

	play_sfx("shot1");
}

TASK(explosive_swirl, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.power = 1)));
	e->move = ARGS.move;

	INVOKE_TASK_WHEN(&e->events.killed, explosive_swirl_explosion, ENT_BOX(e));
}

TASK(supercard_proj, { cmplx pos; Color color; MoveParams move_start; MoveParams move_split; int split_time; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_card,
		.pos = ARGS.pos,
		.color = &ARGS.color,
	));
	p->move = ARGS.move_start;

	WAIT(ARGS.split_time);
	p->color = *RGB(p->color.b, 0.2, p->color.g);
	play_sfx_ex("redirect", 10, false);
	spawn_projectile_highlight_effect(p);
	p->move = ARGS.move_split;
}

TASK(supercard_fairy_move, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);
	WAIT(40);
	e->move.retention = 0.9;
}

TASK(supercard_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(.points = 5)));
	e->move = ARGS.move;
	INVOKE_TASK(supercard_fairy_move, ENT_BOX(e));

	int repeats = 3;
	int bursts = 3;
	int bullets = difficulty_value(20, 40, 40, 50);
	int fan = difficulty_value(1, 1, 2, 2);

	cmplx r = cdir(M_TAU/bursts);

	for(int repeat = 0; repeat < repeats; repeat++) {
		WAIT(70);
		cmplx dirofs = rng_dir();

		for(int burst = 0; burst < bursts; ++burst) {
			for(int i = 0; i < bullets; i++) {
				play_sfx_ex("shot1",5,false);

				cmplx dir = dirofs * cdir(M_TAU / bullets * i + repeat * M_PI/4);

				for(int j = -fan; j <= fan; j++) {
					MoveParams move_split = move_accelerated(dir, 0.02 * (1 + j * I) * dir);
					move_split.retention = 0.99 * cdir(0.02 * -j);

					INVOKE_TASK(supercard_proj,
						.pos = e->pos + 30 * dir,
						.color = *RGB(0, 0.2, 1.0 - 0.2 * psin(i / 20.0)),
						.move_start = move_linear(0.01 * dir),
						.move_split = move_split,
						.split_time = 100 - i
					);
				}

				YIELD;
			}

			WAIT(10);
			dirofs *= r;
		}
	}

	e->move = move_asymptotic_halflife(0, -2*I, 120);
}

TASK(hex_fairy, { cmplx pos; cmplx lattice_vec; MoveParams escape_move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 4)));
	ecls_anyfairy_summon(e, 120);

	int num_flowers = 4;
	WAIT(20);

	real speed = difficulty_value(0.2, 0.2, 0.25, 0.3);

	play_sfx("shot_special1");
	for(int r = 0; r < 6; r++) {
		for(int f = 0; f < num_flowers; f++) {
			cmplx dir = cdir(M_TAU/6 * r)*(1 + I * sin(M_TAU / 6) * (f / (real)(num_flowers-1) - 0.5)) * cnormalize(ARGS.lattice_vec);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGBA(0.8, 0.0, 0.5, 0.0),
				.move = (MoveParams) {speed * dir, 0, 1.005}
			);
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 7 * dir,
				.color = RGBA(1.0, 0.0, 0.5, 0.0),
				.move = (MoveParams) {speed * dir, 0, 1.005}
			);
		}
	}

	WAIT(60);
	e->move = ARGS.escape_move;

}

TASK(spawn_hex_fairy, { cmplx offset; real angle; MoveParams escape_move; }) {
	// spawn a triangular lattice of fairies
	cmplx lattice_vec1 = cdir(ARGS.angle) * difficulty_value(220, 200, 180, 160);
	cmplx lattice_vec2 = cdir(-2*M_TAU / 3) * lattice_vec1;

	cmplx offset = ARGS.offset;
	int Lx = 10;
	int Ly = 20;

	for(int y = -Ly/2; y < Ly/2; y++) {
		for(int x = -Lx/2; x < Lx/2; x++) {
			cmplx pos = offset + lattice_vec1 * x + lattice_vec2 * y;

			INVOKE_TASK(hex_fairy,
				.pos=pos,
				.lattice_vec = lattice_vec1,
				.escape_move = ARGS.escape_move
			);
		}
		WAIT(10);
	}
}


TASK(spiral_fairy, { cmplx pos; cmplx dir; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	ecls_anyfairy_summon(e, 120);

	e->move = move_linear(ARGS.dir);

	int petals = 5;

	Color colors[3] = {
		*RGBA(1.0, 0.0, 0.0, 0.0),
		*RGBA(0.0, 1.0, 0.0, 0.0),
		*RGBA(0.0, 0.0, 1.0, 0.0),
	};

	WAIT(20);
	play_sfx("shot_special1");
	for(int j = 0; j < 3; j++) {

		real angle = rng_angle();
		cmplx dir = rng_dir();

		for(int i = 0; i < petals; i++) {
			cmplx petal_pos = 15 * cdir(M_TAU/petals * i + angle);


			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + petal_pos,
				.color = &colors[j],
				.move = move_linear(1 * dir),
				.angle = carg(petal_pos),
				.flags = PFLAG_MANUALANGLE,
				.max_viewport_dist = 64,
			);
		}
	}
}

TASK(spawn_spiral_fairy, { real twist; }) {
	int arms = 4;
	real link_length = difficulty_value(80, 60, 40, 30);

	cmplx origin = VIEWPORT_W / 2.0 + I * VIEWPORT_H / 2.0 - 90 * I;

	int L = VIEWPORT_H / link_length * 1.5;

	for(int link = 1; link < L; link++) {
		for(int arm = 0; arm < arms; arm++) {
			cmplx pos = origin + link * link_length * cdir(M_TAU/arms * arm + link * ARGS.twist);

			INVOKE_TASK(spiral_fairy, .pos = pos, .dir = I*cnormalize(pos - origin));
		}
		WAIT(10);
	}
}

TASK(laser_pattern_fairy, { cmplx pos; cmplx dir; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points=2)));
	INVOKE_SUBTASK_DELAYED(60, common_charge,
		.time = 90,
		.pos = e->pos,
		.color = *color_mul_scalar(RGBA(0.7, 1.0, 0.2, 0), 0.25),
		.sound = COMMON_CHARGE_SOUNDS,
	);
	ecls_anyfairy_summon(e, 120);
	WAIT(30);
	e->move = move_accelerated(2*ARGS.dir, 0.01*I*ARGS.dir);

	for(;;) {
		int count = 5;
		real length = difficulty_value(20,30,40,50);
		for(int i = 0; i < count; i++) {
			create_laser(e->pos, length, 200, RGBA(0.7, 1.0, 0.2, 0),
				laser_rule_linear(3*ARGS.dir*cdir(M_TAU/count * i)));
		}
		play_sfx("laser1");

		WAIT(60);
	}
}

TASK(spawn_laser_pattern_fairy) {
	int count = difficulty_value(10,12,14,15);

	cmplx pos = VIEWPORT_W * 0.5 + VIEWPORT_H * 0.4 * I;
	for(int i = 0; i < count; i++) {
		cmplx dir = cdir(M_TAU / count * i);

		INVOKE_TASK(laser_pattern_fairy, .pos = pos + 100 * dir, .dir = dir);

		WAIT(10);
	}
}

TASK_WITH_INTERFACE(kurumi_intro, BossAttack) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	b->move = move_from_towards(b->pos, BOSS_DEFAULT_GO_POS, 0.03);
}

TASK_WITH_INTERFACE(kurumi_outro, BossAttack) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	b->move = move_linear(-5 - I);
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK(midboss);

	Boss *b = global.boss = stage4_spawn_kurumi(VIEWPORT_W / 2.0 - 400.0 * I);

	boss_add_attack_task(b, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, kurumi_intro), NULL);
	boss_add_attack_from_info(b, &stage4_spells.mid.gate_of_walachia, false);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.mid.dry_fountain, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.mid.red_spike, false);
	}
	boss_add_attack_task(b, AT_Move, "Outro", 2, 1, TASK_INDIRECT(BossAttack, kurumi_outro), NULL);
	boss_engage(b);
}

TASK(scythe_enter, { BoxedEllyScythe s; }) {
	auto s = TASK_BIND(ARGS.s);
	int entertime = 120;
	for(int t = 0; t < entertime; ++t, YIELD) {
		float f = (t + 1.0f) / entertime;
		s->scale = 0.7f * f * f;
		s->opacity = glm_ease_quad_out(f);
	}
}

TASK(scythe_post_mid, { int fleetime; }) {
	cmplx spawnpos = VIEWPORT_W/2 + 140i;
	EllyScythe *s = stage6_host_elly_scythe(spawnpos);
	INVOKE_SUBTASK(scythe_enter, ENT_BOX(s));
	s->spin = M_TAU/12;

	cmplx anchor = VIEWPORT_W / 2.0 + I * VIEWPORT_H / 3.0;
	s->move = move_from_towards(s->pos, anchor, 0.03 + 0.01i);

	real speed = difficulty_value(1.1, 1.0, 0.9, 0.8);
	real boost = difficulty_value(5, 7, 10, 15);

	WAIT(90);
	s->spin *= -1;

	real hrange = 230;
	real vrange = difficulty_value(80, 100, 120, 140);
	real vofs = 45;

	for(int i = 0; i < ARGS.fleetime - 210; i++, YIELD) {
		s->move.attraction_point = anchor + hrange * sin((i+60) * 0.03) + I*(vofs + vrange * cos((i+60) * 0.05));

		if(i <= 30) {
			continue;
		}

		cmplx shotorg = s->pos + 80 * cdir(s->angle);
		cmplx shotdir = cdir(-1.1 * s->angle);

		struct projentry { ProjPrototype *proj; char *snd; } projs[] = {
			{ pp_ball,       "shot1"},
			{ pp_bigball,    "shot1"},
			{ pp_soul,       "shot_special1"},
			{ pp_bigball,    "shot1"},
		};

		struct projentry *pe = &projs[i % (sizeof(projs)/sizeof(struct projentry))];

		float ca = i/60.0f;
		Color *c = RGBA(cosf(ca), sinf(ca), cosf(ca + 2.1f), 0.5f);

		play_sfx_ex(pe->snd, 3, true);

		PROJECTILE(
			.proto = pe->proj,
			.pos = shotorg,
			.color = c,
			.move = move_asymptotic_simple(speed * shotdir, boost * (1 + psin(i / 150.0))),
			.flags = PFLAG_MANUALANGLE,
			.angle = rng_angle(),
			.angle_delta = M_TAU/64,
		);
	}

	s->move.attraction = re(s->move.attraction);
	s->move.attraction_point = spawnpos;
	s->spin *= -1;

	for(int i = 0; i < 120; i++, YIELD) {
		float f = 1.0f - i / 120.0f;
		s->scale = 0.7f * f * f * f;
		s->opacity = glm_ease_quad_out(f);
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
	}

	if(ARGS.fleetime >= 300) {
		spawn_items(s->pos, ITEM_LIFE, 1);
	}

	stage6_despawn_elly_scythe(s);
}

TASK_WITH_INTERFACE(kurumi_boss_intro, BossAttack){
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	b->move = move_from_towards(b->pos, BOSS_DEFAULT_GO_POS, 0.015);

	WAIT(120);
	stage4_dialog_pre_boss();
}

TASK(spawn_boss) {
	STAGE_BOOKMARK(boss);
	Boss *b = global.boss = stage4_spawn_kurumi(-400 * I);

	stage_unlock_bgm("stage4");
	stage4_bg_redden_corridor();

	boss_add_attack_task(b, AT_Move, "Introduction", 4, 0, TASK_INDIRECT(BossAttack, kurumi_boss_intro), NULL);
	boss_add_attack_task(b, AT_Normal, "", 25, 33000, TASK_INDIRECT(BossAttack, stage4_boss_nonspell_1), NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.boss.animate_wall, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.boss.demon_wall, false);
	}
	boss_add_attack_task(b, AT_Normal, "", 25, 36000, TASK_INDIRECT(BossAttack, stage4_boss_nonspell_2), NULL);
	boss_add_attack_from_info(b, &stage4_spells.boss.bloody_danmaku, false);
	boss_add_attack_from_info(b, &stage4_spells.boss.blow_the_walls, false);
	boss_add_attack_from_info(b, &stage4_spells.extra.vlads_army, false);
	boss_engage(b);
}


DEFINE_EXTERN_TASK(stage4_timeline) {
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_H / 4.0 * 3 * I, 1);
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_W + VIEWPORT_H / 4.0 * 3 * I, -1);

	for(int i = 0; i < 30; i++) {
		real phase = 2 * i / M_PI * (1 - 2 * (i&1));
		cmplx pos = VIEWPORT_W * 0.5 * (1 + 0.5 * sin(phase)) - 32 * I;
		INVOKE_TASK_DELAYED(300 + 15 * i, fodder_fairy, .pos = pos, .move = move_linear(2 * I * cdir(0.1 * phase)));
	}

	for(int i = 0; i < 12; i++) {
		cmplx pos = VIEWPORT_W * (i & 1) + 200 * I;
		cmplx vel = 2 * cdir(M_PI / 2 * (0.2 + 0.6 * rng_real() + (i & 1)));

		INVOKE_TASK_DELAYED(500 + 80 * i, partcircle_fairy, .pos = pos, .move = move_linear(vel));
	}

	for(int i = 0; i < 17; i++) {
		real y = rng_range(200, 300);
		real dy = rng_range(-0.1, 0.1);

		INVOKE_TASK_DELAYED(600 + 40 * i, filler_fairy, .pos = y * I, .move = move_linear(2 + dy * I));
		INVOKE_TASK_DELAYED(600 + 40 * i, filler_fairy, .pos = y * I + VIEWPORT_W, .move = move_linear(-2 + dy * I));
	}

	for(int i = 0; i < 4; i++) {
		cmplx pos0 = VIEWPORT_W * (1.0 + 2.0 * (i & 1)) / 4.0;
		cmplx pos1 = VIEWPORT_W * (1.0 + 4.0 * (i & 1)) / 6.0 + 100 * I;
		cmplx pos2 = VIEWPORT_W * (1.0 + 2.0 * ((i+1) & 1)) / 4.0 + 300 * I;
		cmplx pos3 = VIEWPORT_W * 0.5 + (VIEWPORT_H + 200) * I;

		INVOKE_TASK_DELAYED(600 + 100 * i, cardbuster_fairy, .poss = {
			pos0, pos1, pos2, pos3
		});
	}

	int swirl_delay = difficulty_value(100, 80, 65, 55);
	int swirl_count = 200 / swirl_delay;
	for(int i = 0; i < swirl_count; i++) {
		INVOKE_TASK_DELAYED(1500 + swirl_delay * i, laser_fairy, .pos = VIEWPORT_H / 2.0 * I + 50 * i * I, .move = move_linear(1));
		INVOKE_TASK_DELAYED(1500 + swirl_delay * i, laser_fairy, .pos = VIEWPORT_W + VIEWPORT_H / 2.0 * I + 50 * i * I, .move = move_linear(-1));

	}

	for(int i = 0; i < 30; i++) {
		real phase = 2*i/M_PI * (1 - 2*(i&1));
		cmplx pos = -24 + I * psin(phase) * 300 + 100 * I;
		INVOKE_TASK_DELAYED(1700 + 15 * i, fodder_fairy, .pos = pos, .move = move_linear(2 * cdir(0.005 * phase)));
	}

	for(int i = 0; i < 4; i++) {
		cmplx pos = VIEWPORT_W / 2.0 + 200 * rng_sreal();
		INVOKE_TASK_DELAYED(1600 + 200 * i, bigcircle_fairy, .pos = pos, .vel = 2.0 * I);
	}

	STAGE_BOOKMARK_DELAYED(2200, supercard);

	INVOKE_TASK_DELAYED(2200, supercard_fairy,
		.pos = VIEWPORT_W / 2.0,
		.move = move_linear(4.0 * I)
	);

	for(int i = 0; i < 40; i++) {
		cmplx pos = VIEWPORT_W + I * (20 + VIEWPORT_H / 3.0 * rng_real());
		INVOKE_TASK_DELAYED(2600 + 10 * i, explosive_swirl, .pos = pos, .move = move_linear(-3));
	}

	WAIT(3200);
	INVOKE_TASK(spawn_midboss);
	int midboss_time = WAIT_EVENT(&global.boss->events.defeated).frames;
	int filler_time = STAGE4_MIDBOSS_MUSIC_TIME;

	STAGE_BOOKMARK(post-midboss);

	if(filler_time - midboss_time > 100) {
		INVOKE_TASK(scythe_post_mid, .fleetime = filler_time - midboss_time);
	}

	WAIT(filler_time - midboss_time);
	STAGE_BOOKMARK(post-filler);

	INVOKE_TASK_DELAYED(20, spawn_hex_fairy, .offset = VIEWPORT_W / 2 + I * VIEWPORT_H / 2, .angle = 0.0, .escape_move = move_linear(1 + I));

	// for(int i = 0; i < 4; i++) {
	// 	real phase = 2 * i/M_PI;
	// 	cmplx pos = VIEWPORT_W * (i&1) + I * 120 * psin(phase);
	// 	INVOKE_TASK_DELAYED(280 + 15 * i, laser_fairy, .pos = pos, .move = move_linear(2 - 4 * (i & 1) + I));
	// }
	for(int i = 0; i < 40; i++) {
		cmplx pos = VIEWPORT_W * (i & 1) + I * (20.0 + VIEWPORT_H/3.0 * rng_real());
		cmplx vel = 3 * (1 - 2 * (i & 1)) + I;
		INVOKE_TASK_DELAYED(480 + 10 * i, explosive_swirl, .pos = pos, .move = move_linear(vel));
	}

	INVOKE_TASK_DELAYED(320, spawn_hex_fairy, .offset = VIEWPORT_W / 2 + I * VIEWPORT_H / 2 + I * 100 , .angle = 0.0, .escape_move = move_linear(-1 + I));


	// for(int i = 0; i < 11; i++) {
	// 	cmplx pos = VIEWPORT_W * (i & 1);
	// 	cmplx vel = 2 * cdir(M_PI / 2 * (0.2 + 0.6 * rng_real() + (i & 1)));

	// 	INVOKE_TASK_DELAYED(350 + 60 * i, partcircle_fairy, .pos = pos, .move = move_linear(vel));
	// }

	for(int i = 0; i < 2; i++) {
		cmplx pos0 = VIEWPORT_W * (1.0 + 2.0 * (i & 1)) / 4.0;
		cmplx pos1 = VIEWPORT_W * (0.5 + 0.4 * (1 - 2 * (i & 1))) + 100.0 * I;
		cmplx pos2 = VIEWPORT_W * (1.0 + 2.0 * ((i + 1) & 1)) / 4.0 + 300.0*I;
		cmplx pos3 = VIEWPORT_W * 0.5 - 200 * I;

		INVOKE_TASK_DELAYED(600 + 200 * i, cardbuster_fairy, .poss = {
			pos0, pos1, pos2, pos3
		});
	}

	cmplx pos[4] = {
		100, VIEWPORT_W-100, 0, VIEWPORT_W
	};

	INVOKE_TASK_DELAYED(1200, spawn_laser_pattern_fairy);
	for(int i = 0; i < 4; i++) {
		INVOKE_TASK_DELAYED(1000+120*i, bigcircle_fairy, .pos = pos[i], .vel = cnormalize(global.plr.pos-pos[i]));
	}

	INVOKE_TASK_DELAYED(1500, spawn_spiral_fairy, .twist = 0.1);

	INVOKE_TASK_DELAYED(1600, supercard_fairy,
		.pos = VIEWPORT_W / 2.0,
		.move = move_linear(4.0 * I)
	);

	INVOKE_TASK_DELAYED(2000, spawn_spiral_fairy, .twist = -0.5);

	WAIT(2460);
	INVOKE_TASK(spawn_boss);
	WAIT_EVENT(&NOT_NULL(global.boss)->events.defeated);
	stage_unlock_bgm("stage4boss");

	WAIT(240);
	stage4_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
