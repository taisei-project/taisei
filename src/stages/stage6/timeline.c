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

static void stage6_dialog_pre_final(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage6PreFinalDialog, pm->dialog->Stage6PreFinal);
}

TASK(hacker_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 4, .power = 3)));
	e->move = ARGS.move;

	WAIT(70);
	e->move = move_stop(0.5);

	WAIT(30);
	int duration = difficulty_value(220, 260, 300, 340);
	int step = 3;
	
	for(int i = 0; i < duration/step; i++, WAIT(step)) {
		play_sfx_loop("shot1_loop");
		for(int j = 0; j < 6; j++) {
			cmplx dir = sin(i * 0.2) * cdir(0.3 * (j / 2 - 1)) * (1 - 2 * (i&1));

			cmplx vel = (-0.5 * sin(global.frames + i * 46752 + 16463 * j + 467 * sin(global.frames*i*j))) * global.diff + creal(dir) + 2.0 * I;

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 120 * dir,
				.color = RGB(1.0, 0.2 - 0.01 * i, 0.0),
				.move = move_linear(vel)
			);
		}
	}

	WAIT(60);
	e->move = move_linear(-ARGS.move.velocity);
}

TASK(side_fairy, { cmplx pos; MoveParams move; cmplx direction; real index; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 4, .power = 3)));
	e->move = ARGS.move;

	WAIT(60);
	e->move = move_stop(0.5);

	WAIT(10);

	int count = difficulty_value(25, 35, 45, 55); 
	real speed = difficulty_value(0.9, 1.1, 1.3, 1.5);
	play_sfx_ex("shot1", 4, true);
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
	p->move = ARGS.move;
}


TASK(flowermine_fairy, { cmplx pos; MoveParams move1; MoveParams move2; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 4, .power = 3)));
	INVOKE_TASK(flowermine_fairy_move, ENT_BOX(e), .move1 = ARGS.move1, .move2 = ARGS.move2);

	int step = difficulty_value(6, 5, 4, 3);
	real speed = difficulty_value(1.0, 1.3, 1.6, 1.9);
	for(int i = 0; i < 1000/step; i++, WAIT(step)) {
		cmplx dir = cdir(0.6 * i);
		Projectile *p = PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 40 * cnormalize(ARGS.move1.velocity) * dir,
			.color = RGB(1-psin(i), 0.3, psin(i)),
			.move = move_linear(I * dir * 0.0001)
		);
		INVOKE_TASK_DELAYED(200, projectile_redirect, ENT_BOX(p), move_linear(I * cdir(0.6 * i) * speed));
		play_sfx("shot1");
	}
}

TASK(scythe_mid_aimshot, { BoxedEllyScythe scythe; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	for(int i = 0;; i++, WAIT(2)) {
		cmplx dir = cdir(scythe->angle);

		real speed = difficulty_value(0.01, 0.02, 0.03, 0.04);

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = scythe->pos + 80 * dir,
			.color = RGBA(0, 0.2, 0.5, 0.0),
			.move = move_accelerated(dir, speed * cnormalize(global.plr.pos - scythe->pos - 80 * dir))
		);

		if(projectile_in_viewport(p)) {
			play_sfx("shot1");
		}
	}
}
	

TASK(scythe_mid, { cmplx pos; }) {
	STAGE_BOOKMARK(scythe-mid);
	EllyScythe *s = stage6_host_elly_scythe(ARGS.pos);
	INVOKE_SUBTASK(stage6_elly_scythe_spin, ENT_BOX(s),
		       .angular_velocity = 0.2,
		       .duration = -1);

	real scythe_speed = difficulty_value(5, 4, 2, 1);

	s->move = move_accelerated(scythe_speed, - 0.005 * I);

	if(global.diff > D_Normal) {
		INVOKE_SUBTASK(scythe_mid_aimshot, ENT_BOX(s));
	}
	
	for(int i = 0; i < 300; i++, YIELD) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cdir(s->angle);
		Projectile *p = PROJECTILE(
			.proto = pp_bigball,
			.pos = s->pos + 80 * dir,
			.color = RGBA(0.2, 0.5 - 0.5 * cimag(dir), 0.5 + 0.5 * creal(dir), 0.0),
		);

		INVOKE_TASK_DELAYED(140, projectile_redirect, ENT_BOX(p), move_linear(global.diff * cexp(0.6 * I) * dir));
	}
}

/*
static int scythe_intro(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	TIMER(&t);

	GO_TO(e,VIEWPORT_W/2+200.0*I, 0.05);


	FROM_TO(60, 119, 1) {
		e->args[1] -= 0.00333333*I;
		e->args[2] -= 0.007;
	}

	scythe_common(e, t);
	return 1;
}
*/
TASK_WITH_INTERFACE(elly_intro, ScytheAttack) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);
	boss->move = move_towards_power(BOSS_DEFAULT_GO_POS, 0.3, 0.5);

	EllyScythe *scythe = NOT_NULL(ENT_UNBOX(ARGS.scythe));

	scythe->move = move_towards_power(BOSS_DEFAULT_GO_POS, 0.3, 0.5);
	STALL;
	
}
/*
static void elly_insert_interboss_dialog(Boss *b, int t) {
	stage6_dialog_pre_final();
}

static void elly_begin_toe(Boss *b, int t) {
	TIMER(&t);

	AT(1) {
		STAGE_BOOKMARK(fall-over);
		stage6_bg_start_fall_over();
		stage_unlock_bgm("stage6boss_phase2");
		stage_start_bgm("stage6boss_phase3");
	}
}
*/

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
	boss->move = move_towards_power(BOSS_DEFAULT_GO_POS, 0.3, 0.5);
}

TASK_WITH_INTERFACE(elly_goto_center, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_towards_power(BOSS_DEFAULT_GO_POS, 0.3, 0.5);
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

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK(boss);

	Boss *boss = global.boss = stage6_spawn_elly(-200.0*I);

	BoxedEllyScythe scythe_ref;
	INVOKE_SUBTASK(host_scythe, VIEWPORT_W + 100 + 200 * I, &scythe_ref);

	TASK_IFACE_ARGS_SIZED_PTR_TYPE(ScytheAttack) scythe_args = TASK_IFACE_SARGS(ScytheAttack,
	  .scythe = scythe_ref
	);
	

	PlayerMode *pm = global.plr.mode;
	Stage6PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage6PreBossDialog, pm->dialog->Stage6PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage6boss_phase1");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task_with_args(boss, AT_Move, "Catch the Scythe", 5, 30000, TASK_INDIRECT(ScytheAttack, elly_intro).base, NULL, scythe_args.base);
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
	
	Attack *pshift = boss_add_attack(boss, AT_Move, "Paradigm Shift", 5, 0, NULL, NULL);
	INVOKE_TASK_WHEN(&pshift->events.initiated, stage6_boss_paradigm_shift, 
		.base.boss = ENT_BOX(boss),
		.base.attack = pshift,
		.scythe = scythe_ref,
		.baryons = baryons_ref
	);

	boss_add_attack_from_info_with_args(boss, &stage6_spells.baryon.many_world_interpretation, baryons_args.base);
	
	
	/*
	boss_add_attack(b, AT_Normal, "Baryon", 50, 55000, elly_baryonattack, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.wave_particle_duality, false);
	boss_add_attack_from_info(b, &stage6_spells.baryon.spacetime_curvature, false);
	boss_add_attack(b, AT_Normal, "Baryon2", 50, 55000, elly_baryonattack2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.higgs_boson_uncovered, false);
	boss_add_attack_from_info(b, &stage6_spells.extra.curvature_domination, false);
	boss_add_attack(b, AT_Move, "Explode", 4, 0, elly_baryon_explode, NULL);
	boss_add_attack(b, AT_Move, "Move to center", 4, 0, elly_goto_center, NULL);
	boss_add_attack(b, AT_Immediate, "Final dialog", 0, 0, elly_insert_interboss_dialog, NULL);
	boss_add_attack(b, AT_Move, "ToE transition", 7, 0, elly_begin_toe, NULL);
	boss_add_attack_from_info(b, &stage6_spells.final.theory_of_everything, false);*/
	boss_engage(boss);
	WAIT_EVENT(&global.boss->events.defeated);
	AWAIT_SUBTASKS;

}

DEFINE_EXTERN_TASK(stage6_timeline) {

	INVOKE_TASK_DELAYED(100, hacker_fairy, .pos = VIEWPORT_W / 2.0, .move = move_linear(2.0 * I));

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
		INVOKE_TASK_DELAYED(1380 + 20 * i, flowermine_fairy,
			.pos = 200.0 * I,
			.move1 = move_linear(2 * cdir(0.5 * M_PI / 9 * i) + 1.0),
			.move2 = move_towards_power(-100, 1, 0.2)
		);
	}
	
	for(int i = 0; i < 20; i++) {
		INVOKE_TASK_DELAYED(1600 + 20 * i, flowermine_fairy,
			.pos = VIEWPORT_W / 2.0,
			.move1 = move_linear(2 * I * cdir(0.5 * M_PI / 9 * i) + 1.0 * I),
			.move2 = move_towards_power(VIEWPORT_W + I * VIEWPORT_H * 0.5 + 100, 1, 0.2)
		);
	}


	INVOKE_TASK_DELAYED(2300, scythe_mid, .pos = -100 + I * 300);

	WAIT(3800);
	INVOKE_TASK(spawn_boss);
	/*
	AT(3800) {
		stage_unlock_bgm("stage6");
		global.boss = create_elly();
	}

	AT(3805) {
		stage_unlock_bgm("stage6boss_phase3");
		stage_finish(GAMEOVER_SCORESCREEN);
	}*/
}
