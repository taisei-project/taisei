/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagex.h"

#include "background_anim.h"
#include "draw.h"
#include "spells/spells.h"
#include "timeline.h"   // IWYU pragma: keep
#include "yumemi.h"
#include "scuttle.h""

#include "global.h"
#include "stage.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stagex_spells_s stagex_spells = {
	.midboss = {
		.stack_smashing = {
			{-1, -1, -1, 3}, AT_Spellcard, "“TODO Stack Smashing”", 60, 20000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_stack_smashing),
			stagex_draw_scuttle_spellbg, CMPLX(VIEWPORT_W/2,VIEWPORT_H/2), 7,
		},
		.fork_bomb = {
			{-1, -1, -1, 4}, AT_Spellcard, "IEEE 1003.1-1988 “fork()”", 60, 20000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_fork_bomb),
			stagex_draw_scuttle_spellbg, CMPLX(VIEWPORT_W/2,VIEWPORT_H/2), 7,
		},
	},
	.boss = {
		.infinity_network = {
			{-1, -1, -1, 1}, AT_SurvivalSpell, "Network “Ethereal Ethernet”", 60, 80000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_infinity_network),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+100.0*I, 7,
		},
		.sierpinski = {
			{-1, -1, -1, 2}, AT_Spellcard, "Automaton “Legacy of Sierpiński”", 90, 60000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_sierpinski),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.mem_copy = {
			{-1, -1, -1, 5}, AT_Spellcard, "Memory “Block-wise Copy”", 90, 150000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_mem_copy),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.pipe_dream = {
			{-1, -1, -1, 6}, AT_Spellcard, "Philosophy “Pipe Dream”", 90, 150000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_pipe_dream),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.alignment = {
			{-1, -1, -1, 7}, AT_Spellcard, "Address Space “Pointer Alignment”", 90, 80000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_alignment),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.rings = {
			{-1, -1, -1, 8}, AT_Spellcard, "Protection Domain “Ring ∞”", 90, 80000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_rings),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.dataflow = {
			{-1, -1, -1, 9}, AT_Spellcard, "Data Flow “Full-Duplex Transmission”", 120, 100000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_dataflow),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7,
		},
		.garbage = {
			{-1, -1, -1, 10}, AT_Spellcard, "Memory “Tracing Garbage Collector”", 90, 150000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_garbage),
			stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+80.0*I, 7,
		},
	},
};

static void stagex_begin(void) {
	stagex_drawsys_init();
	stagex_bg_init_fullstage();
	stage_start_bgm("stagex");

	INVOKE_TASK(stagex_timeline);
}

static void stagex_spellpractice_begin(void) {
	stagex_drawsys_init();

	if(global.stage->spell->draw_rule == stagex_draw_scuttle_spellbg) {
		global.boss = stagex_spawn_scuttle(BOSS_DEFAULT_SPAWN_POS);
		stage_unlock_bgm("scuttle");
		stage_start_bgm("scuttle");
		stagex_bg_init_practice_midboss();
	} else {
		global.boss = stagex_spawn_yumemi(BOSS_DEFAULT_SPAWN_POS);
		stage_start_bgm("stagexboss");
		stagex_bg_init_practice_boss();
	}

	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);
}

static void stagex_end(void) {
	stagex_drawsys_shutdown();
}

Boss *stagex_init_boss_attack(StageXBossAttackTaskArgs *args) {
	if(!args->corruption) {
		args->corruption = stagex_corruption_create();
	}

	return INIT_BOSS_ATTACK(&args->base);
}

static void stagex_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"cell_noise",
		"stagex/bg",
		"stagex/bg_binary",
		"stagex/code",
		"stagex/dissolve_mask",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"extra_bg",
		"extra_tower_apply_mask",
		"extra_tower_mask",
		"zbuf_fog",
	NULL);
	res_group_preload(rg, RES_MATERIAL, RESF_DEFAULT,
		"stage5/metal",
		"stage5/stairs",
		"stage5/wall",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"stage5/metal",
		"stage5/stairs",
		"stage5/wall",
		"tower_alt_uv",
	NULL);
}

StageProcs stagex_procs = {
	.begin = stagex_begin,
	.end = stagex_end,
	.draw = stagex_draw_background,
	.preload = stagex_preload,
	.shader_rules = stagex_bg_effects,
	.postprocess_rules = stagex_postprocess_effects,
	.spellpractice_procs = &stagex_spell_procs,
};

StageProcs stagex_spell_procs = {
	.begin = stagex_spellpractice_begin,
	.end = stagex_end,
	.draw = stagex_draw_background,
	.preload = stagex_preload,
	.shader_rules = stagex_bg_effects,
	.postprocess_rules = stagex_postprocess_effects,
};
