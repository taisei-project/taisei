/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"   // IWYU pragma: keep
#include "draw.h"
#include "nonspells/nonspells.h"
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
	Attack *opening_attack = boss_add_attack(boss, AT_Normal, "Opening", 60, 40000, NULL, NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.sierpinski, false);
	boss_add_attack_from_info(boss, &stagex_spells.boss.infinity_network, false);

	StageExPreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, yumemi_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");
	INVOKE_TASK_WHEN(&e->music_changes, yumemi_opening, ENT_BOX(boss), opening_attack);
	WAIT_EVENT_OR_DIE(&global.dialog->events.fadeout_began);
#else
	player_set_power(plr, PLR_MAX_POWER_EFFECTIVE);
	player_add_lives(plr, PLR_MAX_LIVES);
// 	boss_add_attack_task(boss, AT_Normal, "non1", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_1), NULL);
// 	boss_add_attack_task(boss, AT_Normal, "non2", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_2), NULL);
// 	boss_add_attack_task(boss, AT_Normal, "non3", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_3), NULL);
// 	boss_add_attack_task(boss, AT_Normal, "non4", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_4), NULL);
// 	boss_add_attack_task(boss, AT_Normal, "non5", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_5), NULL);
	boss_add_attack_task(boss, AT_Normal, "non6", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_6), NULL);
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

// attr_unused
enemies:
	for(int i = 0;;i++) {
		INVOKE_TASK_DELAYED(60, glider_fairy, 2000, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(50+100*(i&1));
	}
}
