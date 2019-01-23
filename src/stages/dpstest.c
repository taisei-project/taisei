/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "dpstest.h"
#include "global.h"
#include "enemy.h"

#define DPSTEST_HP 90000

static void dpstest_stub_proc(void) { }

static int dpstest_dummy(Enemy *e, int t) {
	e->hp = DPSTEST_HP;

	if(t > 0) {
		e->pos += e->args[0];
	}

	return ACTION_NONE;
}

static void stage_dpstest_single_events(void) {
	TIMER(&global.timer);

	AT(0) {
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_H/3*I, DPSTEST_HP, BigFairy, dpstest_dummy, 0);
	}
}

static void stage_dpstest_multi_events(void) {
	TIMER(&global.timer);

	AT(0) {
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_H/3*I, DPSTEST_HP, BigFairy, dpstest_dummy, 0);
		create_enemy1c(-64 + VIEWPORT_W/2 + VIEWPORT_H/3*I, DPSTEST_HP, Fairy, dpstest_dummy, 0);
		create_enemy1c(+64 + VIEWPORT_W/2 + VIEWPORT_H/3*I, DPSTEST_HP, Fairy, dpstest_dummy, 0);
	}

	if(!(global.timer % 16)) {
		create_enemy1c(-16 + VIEWPORT_H/5*I, DPSTEST_HP, Swirl, dpstest_dummy,  4);
	}

	if(!((global.timer + 8) % 16)) {
		create_enemy1c(VIEWPORT_W+16 + (VIEWPORT_H/5 - 32)*I, DPSTEST_HP, Swirl, dpstest_dummy, -4);
	}
}

static void stage_dpstest_boss_rule(Boss *b, int t) {
	if(t >= 0) {
		double x = pow((b->current->maxhp - b->current->hp) / b->current->maxhp, 0.75) * b->current->maxhp;
		b->current->hp = clamp(b->current->hp + x * 0.0025, b->current->maxhp * 0.05, b->current->maxhp);
	}
}

static void stage_dpstest_boss_events(void) {
	TIMER(&global.timer);

	AT(0) {
		global.boss = create_boss("Baka", "cirno", NULL, BOSS_DEFAULT_GO_POS);
		boss_add_attack(global.boss, AT_Move, "", 1, DPSTEST_HP, stage_dpstest_boss_rule, NULL);
		boss_add_attack(global.boss, AT_Spellcard, "Masochism ~ Eternal Torment", 5184000, DPSTEST_HP, stage_dpstest_boss_rule, NULL);
	}
}

StageProcs stage_dpstest_single_procs = {
	.begin = dpstest_stub_proc,
	.preload = dpstest_stub_proc,
	.end = dpstest_stub_proc,
	.draw = dpstest_stub_proc,
	.update = dpstest_stub_proc,
	.event = stage_dpstest_single_events,
	.shader_rules = (ShaderRule[]) { NULL },
};

StageProcs stage_dpstest_multi_procs = {
	.begin = dpstest_stub_proc,
	.preload = dpstest_stub_proc,
	.end = dpstest_stub_proc,
	.draw = dpstest_stub_proc,
	.update = dpstest_stub_proc,
	.event = stage_dpstest_multi_events,
	.shader_rules = (ShaderRule[]) { NULL },
};

StageProcs stage_dpstest_boss_procs = {
	.begin = dpstest_stub_proc,
	.preload = dpstest_stub_proc,
	.end = dpstest_stub_proc,
	.draw = dpstest_stub_proc,
	.update = dpstest_stub_proc,
	.event = stage_dpstest_boss_events,
	.shader_rules = (ShaderRule[]) { NULL },
};
