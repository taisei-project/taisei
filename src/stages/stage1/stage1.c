/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage1.h"
#include "draw.h"
#include "background_anim.h"
#include "cirno.h"
#include "spells/spells.h"
#include "timeline.h"

#include "global.h"
#include "portrait.h"
#include "common_tasks.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage1_spells_s stage1_spells = {
	.mid = {
		.perfect_freeze = {
			{-1, -1,  2,  3}, AT_Spellcard, "Freeze Sign “Perfect Freeze”", 50, 24000,
			NULL, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I, 1,
			TASK_INDIRECT_INIT(BossAttack, stage1_spell_perfect_freeze)
		},
	},

	.boss = {
		.crystal_rain = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Freeze Sign “Crystal Rain”", 40, 33000,
			NULL, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I, 1,
			TASK_INDIRECT_INIT(BossAttack, stage1_spell_crystal_rain)
		},
		.snow_halation = {
			{-1, -1, 12, 13}, AT_Spellcard, "Winter Sign “Snow Halation”", 50, 40000,
			NULL, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I, 1,
			TASK_INDIRECT_INIT(BossAttack, stage1_spell_snow_halation)
		},
		.icicle_cascade = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Doom Sign “Icicle Cascade”", 40, 40000,
			NULL, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I, 1,
			TASK_INDIRECT_INIT(BossAttack, stage1_spell_icicle_cascade)
		},
	},

	.extra.crystal_blizzard = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Frost Sign “Crystal Blizzard”", 60, 40000,
		NULL, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I, 1,
		TASK_INDIRECT_INIT(BossAttack, stage1_spell_crystal_blizzard)
	},
};

#ifdef SPELL_BENCHMARK
AttackInfo stage1_spell_benchmark = {
	{-1, -1, -1, -1, 127}, AT_SurvivalSpell, "Profiling “ベンチマーク”", 40, 40000,
	stage1_spell_benchmark_proc, stage1_draw_cirno_spellbg, VIEWPORT_W/2.0+100.0*I
};
#endif

static void stage1_start(void) {
	stage1_drawsys_init();
	stage1_bg_init_fullstage();
	stage_start_bgm("stage1");
	stage_set_voltage_thresholds(50, 125, 300, 600);
	INVOKE_TASK(stage1_timeline);
}

static void stage1_spellpractice_start(void) {
	stage1_drawsys_init();
	stage1_bg_init_spellpractice();

	Boss *cirno = stage1_spawn_cirno(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(cirno, global.stage->spell, true);
	boss_engage(cirno);
	global.boss = cirno;

	stage_start_bgm("stage1boss");

	INVOKE_TASK_WHEN(&cirno->events.defeated, common_call_func, stage1_bg_disable_snow);
}

static void stage1_preload(void) {
	// DIALOG_PRELOAD(&global.plr, Stage1PreBoss, RESF_DEFAULT);
	portrait_preload_base_sprite("cirno", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite("cirno", "normal", RESF_DEFAULT);
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage1", "stage1boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage1/cirnobg",
		"stage1/fog",
		"stage1/snowlayer",
		"stage1/waterplants",
	NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage1/horizon",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"blur5",
		"stage1_water",
		"zbuf_fog",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/cirno",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
	NULL);
}

static void stage1_end(void) {
	stage1_drawsys_shutdown();
}

StageProcs stage1_procs = {
	.begin = stage1_start,
	.draw = stage1_draw,
	.end = stage1_end,
	.preload = stage1_preload,
	.shader_rules = stage1_bg_effects,
	.spellpractice_procs = &(StageProcs) {
		.begin = stage1_spellpractice_start,
		.draw = stage1_draw,
		.end = stage1_end,
		.preload = stage1_preload,
		.shader_rules = stage1_bg_effects,
	},
};
