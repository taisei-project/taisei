/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "dpstest.h"

#include "enemy_classes.h"
#include "enemy.h"
#include "global.h"

TASK(single_fairy) {
	for(;;) {
		Enemy *e = espawn_super_fairy(VIEWPORT_W/2, ITEMS(.points = 10));
		e->move = move_from_towards(e->pos, BOSS_DEFAULT_GO_POS, 0.025);
		WAIT_EVENT(&e->events.killed);
	}
}

static void dpstest_single(void) {
	INVOKE_TASK(single_fairy);
}

TASK(circling_fairy, { EnemySpawner spawner; cmplx circle_orig; cmplx circle_ofs; }) {
	WAIT(20);

	Enemy *e = TASK_BIND(ARGS.spawner(VIEWPORT_W/2, NULL));

	cmplx org = ARGS.circle_orig;
	cmplx ofs = ARGS.circle_ofs;

	e->move = move_towards(0, 0, 0.025);

	INVOKE_TASK_AFTER(&e->events.killed, circling_fairy, ARGS.spawner, org, ofs);

	for(;;YIELD) {
		e->move.attraction_point = org + ofs * cdir(global.frames * 0.02);
	}
}

static void dpstest_multi(void) {
	EnemySpawner spawners[] = {
		espawn_fairy_blue,
		espawn_fairy_red,
		espawn_big_fairy,
		espawn_huge_fairy,
		espawn_super_fairy,
	};

	for(int i = 0; i < ARRAY_SIZE(spawners); ++i) {
		INVOKE_TASK(circling_fairy,
			spawners[i], VIEWPORT_W/2 + I*VIEWPORT_H/3,
			128 * cdir(i * M_TAU / ARRAY_SIZE(spawners))
		);
	}
}

TASK_WITH_INTERFACE(boss_regen, BossAttack) {
	INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	Attack *a = ARGS.attack;

	for(;;YIELD) {
		real x = pow((a->maxhp - a->hp) / a->maxhp, 0.75) * a->maxhp;
		a->hp = clamp(a->hp + x * 0.0025, a->maxhp * 0.05, a->maxhp);
	}
}

static void dpstest_boss(void) {
	global.boss = create_boss("Baka", "cirno", BOSS_DEFAULT_GO_POS);
	boss_add_attack_task(
		global.boss, AT_Spellcard, "Masochism “Eternal Torment”",
		5184000, 90000, TASK_INDIRECT(BossAttack, boss_regen), NULL
	);
	boss_engage(global.boss);
}

StageProcs stage_dpstest_single_procs = {
	.begin = dpstest_single,
	.shader_rules = (ShaderRule[]) { NULL },
};

StageProcs stage_dpstest_multi_procs = {
	.begin = dpstest_multi,
	.shader_rules = (ShaderRule[]) { NULL },
};

StageProcs stage_dpstest_boss_procs = {
	.begin = dpstest_boss,
	.shader_rules = (ShaderRule[]) { NULL },
};
