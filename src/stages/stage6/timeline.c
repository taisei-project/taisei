/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage6.h"
#include "draw.h"
#include "background_anim.h"
#include "enemy_classes.h"

#include "elly.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"

#include "common_tasks.h"

TASK(hacker_fairy, { cmplx pos; MoveParams exit_move; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(
		.points = 12,
		.power = 6,
		.life = 1,
	)));

	int summon_time = 180;
	int precharge_time = 30;
	int charge_time = 60;

	INVOKE_SUBTASK_DELAYED(summon_time - precharge_time, common_charge,
		.anchor = &e->pos,
		.color = RGBA(1, 0, 0, 0),
		.time = charge_time + precharge_time,
		.sound = COMMON_CHARGE_SOUNDS,
	);

	ecls_anyfairy_summon(e, summon_time);
	WAIT(charge_time);

	int duration = difficulty_value(220, 260, 300, 340);
	int step = 3;

	int density = difficulty_value(3,3,4,5);

	for(int i = 0; i < duration/step; i++, WAIT(step)) {
		play_sfx_loop("shot1_loop");
		for(int j = 0; j <= density; j++) {
			cmplx dir = sin(i * 0.2) * cdir(0.15 * (j - density*0.5)) * (1 - 2 * (i&1));

			cmplx vel = (-0.5 * sin(global.frames + i * 46752 + 16463 * j + 467 * sin(global.frames*i*j))) * global.diff + re(dir) + 2.0 * I;

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 120 * dir,
				.color = RGB(1.0, 0.2 - 0.01 * i, 0.0),
				.move = move_linear(vel)
			);
		}
	}

	WAIT(300);
	e->move = ARGS.exit_move;
}

TASK(side_fairy, { cmplx pos; MoveParams move; cmplx direction; real index; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 2)));
	e->move = ARGS.move;

	WAIT(60);
	e->move = move_dampen(e->move.velocity, 0.8);

	WAIT(10);

	int count = difficulty_value(25, 35, 45, 55);
	real speed = difficulty_value(0.9, 1.1, 1.3, 1.5);
	play_sfx("shot2");
	for(int i = 0; i < count; i++) {
		PROJECTILE(
			.proto = (i % 2 == 0) ? pp_rice : pp_flea,
			.pos = e->pos + 5 * (i/2) * ARGS.direction,
			.color = RGB(0.1 * ARGS.index, 0.5, 1),
			.move = move_accelerated(I * (1.0 - 2.0 * (i & 1)) * speed,
				0.001 * (i/2) * ARGS.direction)
		);
	}

	WAIT(50);
	e->move = ARGS.move;
}

TASK(flowermine_fairy_move, { BoxedEnemy enemy; MoveParams move1; MoveParams move2; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);
	e->move = ARGS.move1; // update me while retaining velocity
	WAIT(70);
	e->move = ARGS.move2;
}

TASK(projectile_redirect, { BoxedProjectile proj; MoveParams move; }) {
	Projectile *p = TASK_BIND(ARGS.proj);
	play_sfx_ex("redirect", 1, false);
	spawn_projectile_highlight_effect(p);
	p->move = ARGS.move;
}

TASK(flowermine_fairy, { cmplx pos; MoveParams move1; MoveParams move2; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	INVOKE_TASK(flowermine_fairy_move, ENT_BOX(e), .move1 = ARGS.move1, .move2 = ARGS.move2);

	int step = difficulty_value(6, 5, 4, 3);
	real speed = difficulty_value(1.0, 1.3, 1.6, 1.9);
	real boost = difficulty_value(0, 3, 5, 7);

	for(int i = 0; i < 1000/step; i++, WAIT(step)) {
		cmplx dir = cdir(0.6 * i);
		Projectile *p = PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 40 * cnormalize(ARGS.move1.velocity) * dir,
			.color = RGB(1-psin(i), 0.1, psin(i)),
			.move = move_linear(I * dir * 0.0001)
		);
		INVOKE_TASK_DELAYED(200, projectile_redirect,
			.proj = ENT_BOX(p),
			.move = move_asymptotic_simple(I * cdir(0.6 * i) * speed, boost)
		);
		play_sfx_loop("shot3");
	}
}

TASK(sniper_fairy_shot_cleanup, { BoxedLaser l; }) {
	auto l = ENT_UNBOX(ARGS.l);
	if(l) {
		l->deathtime = imin(l->deathtime, global.frames - l->birthtime + 10);
	}
}

TASK(sniper_fairy_bullet, { cmplx pos; cmplx aim; }) {
	int vpdist = 64;

	real speed = difficulty_value(30, 20, 12, 6);
	real trail_speed = difficulty_value(2, 2.1, 2.3, 2.5);
	real freq = 1/3.0;

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_bullet,
		.move = move_linear(speed * ARGS.aim),
		.color = RGB(1, 0, 0),
		.pos = ARGS.pos,
		.max_viewport_dist = vpdist,
	));

	play_sfx("shot_special1");

	cmplx dir = cdir(freq/7);
	cmplx r = cdir(freq);

	for(;;) {
		YIELD;

		PROJECTILE(
			.proto = pp_flea,
			.color = RGB(0.5, 0, 1.0),
			.pos = p->pos,
			.move = move_asymptotic_halflife(trail_speed * dir, trail_speed / dir, 10),
			.max_viewport_dist = vpdist,
		);

		dir *= r;

		PROJECTILE(
			.proto = pp_flea,
			.color = RGB(0.5, 0, 1.0),
			.pos = (p->pos + p->prevpos) * 0.5,
			.move = move_asymptotic_halflife(trail_speed / dir, trail_speed * dir, 10),
			.max_viewport_dist = vpdist,
		);

		dir *= r;
	}
}

TASK(sniper_fairy_shot, { BoxedEnemy e; }) {
	auto e = TASK_BIND(ARGS.e);

	int aimtime = 150;
	int aimfreezetime = aimtime - difficulty_value(30, 30, 20, 15);
	int trailtime = 20;
	float lwidth = 16;

	BoxedLaser lb = ENT_BOX(create_laserline(
		e->pos, lwidth, aimtime, aimtime + trailtime, RGBA(1, 0.1, 0.1, 0)));

	BoxedTask cleanup = cotask_box(INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished,
		sniper_fairy_shot_cleanup, lb));

	cmplx aim = global.plr.pos - e->pos;
	MoveParams aim_move = { };

	INVOKE_SUBTASK(common_charge,
		.anchor = &e->pos,
		.time = aimtime,
		.color = RGBA(1, 0.1, 0.1, 0),
		.sound = COMMON_CHARGE_SOUNDS,
	);

	cmplx aim_target = 0;

	for(int t = 0; t < aimtime; ++t, YIELD) {
		Laser *l = ENT_UNBOX(lb);

		if(!l) {
			return;
		}

		if(t < aimfreezetime) {
			aim_target = global.plr.pos;
		}

		aim_move = move_towards(aim_move.velocity, aim_target, 1.5);
		aim_move.retention = 0.8;
		move_update(&aim, &aim_move);
		laserline_set_posdir(l, e->pos, aim - e->pos);
	}

	INVOKE_TASK(sniper_fairy_bullet,
		.pos = e->pos,
		.aim = cnormalize(aim - e->pos),
	);

	play_sfx("laser1");
	CANCEL_TASK(cleanup);
}

#define SNIPER_SPAWNTIME 180

TASK(sniper_fairy, { cmplx pos; MoveParams move_exit; }) {
	auto e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 5, .power = 4)));
	ecls_anyfairy_summon(e, SNIPER_SPAWNTIME);

	INVOKE_SUBTASK(sniper_fairy_shot, ENT_BOX(e));
	AWAIT_SUBTASKS;
	WAIT(20);

	e->move = ARGS.move_exit;
}

TASK(scythe_mid_aimshot, { BoxedEllyScythe scythe; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	real speed = difficulty_value(0.01, 0.02, 0.03, 0.04);
	real r = 80;

	for(;;WAIT(2)) {
		cmplx dir = cdir(scythe->angle);

		PROJECTILE(
			.proto = pp_ball,
			.pos = scythe->pos + r * dir,
			.color = RGBA(0, 0.2, 0.5, 0.0),
			.move = move_accelerated(dir, speed * cnormalize(global.plr.pos - scythe->pos - r * dir)),
			.max_viewport_dist = r,
		);

		play_sfx("shot1");
	}
}

TASK(scythe_mid, { cmplx pos; }) {
	STAGE_BOOKMARK(scythe-mid);
	EllyScythe *s = stage6_host_elly_scythe(ARGS.pos);
	s->spin = 0.2;

	real scythe_speed = difficulty_value(5, 4, 3, 2);
	int firetime = difficulty_value(150, 200, 300, 300);
	real r = 80;

	s->move = move_asymptotic_halflife(scythe_speed, -(scythe_speed*2)*I, firetime);

	if(global.diff > D_Normal) {
		INVOKE_SUBTASK(scythe_mid_aimshot, ENT_BOX(s));
	}

	for(int i = 0; i < firetime; i++, YIELD) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cdir(s->angle);

		Projectile *p = PROJECTILE(
			.proto = pp_bigball,
			.pos = s->pos + r * dir,
			.color = RGBA(0.2, 0.5 - 0.5 * im(dir), 0.5 + 0.5 * re(dir), 0.0),
			.max_viewport_dist = r*2,
		);

		INVOKE_TASK_DELAYED(140, projectile_redirect, ENT_BOX(p), move_linear(global.diff * cdir(0.6) * dir));
	}
}

TASK_WITH_INTERFACE(elly_begin_toe, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);

	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage6PreFinalDialog, pm->dialog->Stage6PreFinal);
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	BEGIN_BOSS_ATTACK(&ARGS);

	boss->move = move_from_towards(boss->pos, ELLY_TOE_TARGET_POS, 0.01);

	stage6_bg_start_fall_over();
	stage_unlock_bgm("stage6boss_phase2");
	stage_start_bgm("stage6boss_phase3");
}

TASK(boss_appear, { BoxedBoss boss; BoxedEllyScythe scythe; }) {
	Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
	EllyScythe *scythe = NOT_NULL(ENT_UNBOX(ARGS.scythe));
	scythe->spin = 0.5;
	boss->move = move_from_towards_exp(boss->pos, BOSS_DEFAULT_GO_POS, 0.03, 0.5);
	WAIT(80);
	scythe->move = move_from_towards_exp(boss->pos, BOSS_DEFAULT_GO_POS + ELLY_SCYTHE_RESTING_OFS, 0.3, 0.5);
	WAIT(35);
	scythe->spin = 0;
}

TASK_WITH_INTERFACE(elly_goto_center, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards_exp(boss->pos, BOSS_DEFAULT_GO_POS, 0.3, 0.5);
}

TASK(host_scythe, { cmplx pos; BoxedEllyScythe *out_scythe; }) {
	EllyScythe *scythe = stage6_host_elly_scythe(ARGS.pos);
	*ARGS.out_scythe = ENT_BOX(scythe);
	WAIT_EVENT(&scythe->events.despawned);
}

TASK(host_baryons, { BoxedBoss boss; BoxedEllyBaryons *out_baryons; }) {
	EllyBaryons *baryons = stage6_host_elly_baryons(ARGS.boss);
	*ARGS.out_baryons = ENT_BOX(baryons);
	WAIT_EVENT(&baryons->events.despawned);
}

TASK(stop_boss_rotation) {
	stage6_bg_stop_boss_rotation();
}

TASK(spawn_boss) {
	STAGE_BOOKMARK(boss);
	stage_unlock_bgm("stage6");

	Boss *boss = global.boss = stage6_spawn_elly(-200.0*I);

	BoxedEllyScythe scythe_ref;
	INVOKE_SUBTASK(host_scythe, VIEWPORT_W + 100 + 200 * I, &scythe_ref);

	TASK_IFACE_ARGS_SIZED_PTR_TYPE(ScytheAttack) scythe_args = TASK_IFACE_SARGS(ScytheAttack,
		.scythe = scythe_ref
	);

	PlayerMode *pm = global.plr.mode;
	Stage6PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage6PreBossDialog, pm->dialog->Stage6PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss), scythe_ref);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage6boss_phase1");

	int scythe_ready_time = 220;
	int dialog_time = WAIT_EVENT(&global.dialog->events.fadeout_began).frames;
	WAIT(scythe_ready_time - dialog_time);
	stage6_bg_start_boss_rotation();

	boss_add_attack_task_with_args(boss, AT_Normal, "Frequency", 40, 50000, TASK_INDIRECT(ScytheAttack, stage6_boss_nonspell_1).base, NULL, scythe_args.base);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.scythe.occams_razor, scythe_args.base);
	boss_add_attack_task_with_args(boss, AT_Normal, "Frequency2", 40, 50000, TASK_INDIRECT(ScytheAttack, stage6_boss_nonspell_2).base, NULL, scythe_args.base);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.scythe.orbital_clockwork, scythe_args.base);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.scythe.wave_theory, scythe_args.base);

	BoxedEllyBaryons baryons_ref;
	INVOKE_SUBTASK(host_baryons, ENT_BOX(boss), &baryons_ref);

	TASK_IFACE_ARGS_SIZED_PTR_TYPE(BaryonsAttack) baryons_args = TASK_IFACE_SARGS(BaryonsAttack,
	  .baryons = baryons_ref
	);

	Attack *pshift = boss_add_attack(boss, AT_Move, "Paradigm Shift", 5, 0, NULL);
	INVOKE_TASK_WHEN(&pshift->events.initiated, stage6_boss_paradigm_shift,
		.base.boss = ENT_BOX(boss),
		.base.attack = pshift,
		.scythe = scythe_ref,
		.baryons = baryons_ref
	);

	boss_add_attack_from_info_with_args(boss, &stage6_spells.baryon.many_world_interpretation, baryons_args.base);
	boss_add_attack_task_with_args(boss, AT_Normal, "Baryon", 50, 55000, TASK_INDIRECT(BaryonsAttack, stage6_boss_nonspell_4).base, NULL, baryons_args.base);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.baryon.wave_particle_duality, baryons_args.base);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.baryon.spacetime_curvature, baryons_args.base);
	boss_add_attack_task_with_args(boss, AT_Normal, "Baryon2", 50, 55000, TASK_INDIRECT(BaryonsAttack, stage6_boss_nonspell_5).base, NULL, baryons_args.base);

	Attack *higgs = boss_add_attack_from_info_with_args(boss, &stage6_spells.baryon.higgs_boson_uncovered, baryons_args.base);
	INVOKE_TASK_WHEN(&higgs->events.started, stop_boss_rotation);
	boss_add_attack_from_info_with_args(boss, &stage6_spells.extra.curvature_domination, baryons_args.base);
	boss_add_attack_task_with_args(boss, AT_Move, "Explode", 4, 0, TASK_INDIRECT(BaryonsAttack, stage6_boss_baryons_explode).base, NULL, baryons_args.base);
	boss_add_attack_task(boss, AT_Move, "Move to center", 4, 0, TASK_INDIRECT(BossAttack, elly_goto_center), NULL);

	boss_add_attack_task(boss, AT_Move, "ToE transition", 7, 0, TASK_INDIRECT(BossAttack, elly_begin_toe), NULL);
	boss_add_attack_from_info(boss, &stage6_spells.final.theory_of_everything, false);
	boss_engage(boss);
	WAIT_EVENT(&global.boss->events.defeated);
	AWAIT_SUBTASKS;

}

TASK(wallmaker, { cmplx pos; MoveParams move; }) {
	auto e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 1)));
	e->move = ARGS.move;

	int period = difficulty_value(7, 6, 5, 4);
	int n = 1;

	for(;;WAIT(period)) {
		play_sfx_loop("shot1_loop");

		for(int i = 0; i < n; ++i) {
			real ox = rng_sreal() * 8;
			real oy = 24;
			real b = rng_range(1, 3);
			PROJECTILE(
				.proto = pp_wave,
				.color = RGB(0.2, 0.0, 0.5),
				.pos = e->pos + ox + oy*I,
				.move = move_asymptotic_simple(I, b),
			);
		}
	}
}

TASK(wallmakers, { int count; cmplx pos; MoveParams move; }) {
	for(int i = 0; i < ARGS.count; ++i, WAIT(60)) {
		INVOKE_TASK(wallmaker, ARGS.pos, ARGS.move);
	}
}

DEFINE_EXTERN_TASK(stage6_timeline) {
	INVOKE_TASK_DELAYED(20, hacker_fairy, {
		.pos = VIEWPORT_W / 2.0 + 200 * I,
		.exit_move = move_asymptotic_simple(2.0 * I, -1),
	});

	for(int i = 0; i < 20; i++) {
		INVOKE_TASK_DELAYED(500 + 10 * i, side_fairy,
			.pos = VIEWPORT_W * (i & 1),
			.move = move_linear(2.0 * I + 0.1 * (1 - 2 * (i & 1))),
			.direction = 1 - 2 * (i & 1),
			.index = i
		);
	}

	for(int i = 0; i < 22; i++) {
		cmplx pos = VIEWPORT_W / 2.0 + (1 - 2*(i&1)) * 20 * (i % 10);
		INVOKE_TASK_DELAYED(720 + 10 * i, side_fairy,
			.pos = pos,
			.move = move_linear(2.0 * I + (1 - 2 * (i & 1))),
			.direction = I * cnormalize(global.plr.pos - pos) * (1 - 2 * (i & 1)),
			.index = i * psin(i)
		);
	}

	for(int i = 0; i < 14; i++) {
		INVOKE_TASK_DELAYED(980 + 20 * i, flowermine_fairy,
			.pos = 200.0 * I,
			.move1 = move_linear(2 * cdir(0.5 * M_PI / 9 * i) + 1.0),
			.move2 = move_towards_exp(0, -100, 1, 0.2)
		);
	}

	INVOKE_TASK_DELAYED(1500, wallmakers, 20, VIEWPORT_W+30+40i, move_linear(-1));
	INVOKE_TASK_DELAYED(1740, wallmakers, 16, -30+40i, move_linear(1));

	for(int i = 0; i < 20; i++) {
		INVOKE_TASK_DELAYED(1200 + 20 * i, flowermine_fairy,
			.pos = VIEWPORT_W / 2.0,
			.move1 = move_linear(2 * I * cdir(0.5 * M_PI / 9 * i) + 1.0 * I),
			.move2 = move_towards_exp(0, VIEWPORT_W + I * VIEWPORT_H * 0.5 + 100, 1, 0.2)
		);
	}

	for(int i = 0; i < 20; i++) {
		INVOKE_TASK_DELAYED(1420 + 20 * i, flowermine_fairy,
			.pos = 200.0 * I,
			.move1 = move_linear(3),
			.move2 = move_asymptotic_halflife(3, 2*I, 60),
		);
	}

	int sniper_step = difficulty_value(3,2,2,1);
	for(int i = 0; i < 12; i += sniper_step) {
		INVOKE_TASK_DELAYED(1400 + 120 * i - SNIPER_SPAWNTIME, sniper_fairy,
			.pos = VIEWPORT_W + VIEWPORT_H*0.6i - 1.5 * 80 * cdir(0.5 * ((2 - (i % 3)) - 1)),
			.move_exit = move_accelerated(-1, -0.04i),
		);
	}

	for(int i = 0; i < 6; i += sniper_step) {
		INVOKE_TASK_DELAYED(2180 + 120 * i - SNIPER_SPAWNTIME, sniper_fairy,
			.pos = VIEWPORT_H*0.6i + 1.5 * 80 * cdir(0.5 * ((2 - (i % 3)) - 1)),
			.move_exit = move_accelerated(1, -0.04i),
		);
	}

	INVOKE_TASK_DELAYED(3300, scythe_mid, .pos = -100 + I * 300);

	WAIT(3800);
	INVOKE_TASK(spawn_boss);
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(300);

	stage_unlock_bgm("stage6boss_phase3");

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
