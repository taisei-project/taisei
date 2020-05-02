/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stagex.h"
#include "yumemi.h"
#include "draw.h"
#include "background_anim.h"
#include "timeline.h"

#include "coroutine.h"
#include "global.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stagex_spells_s stagex_spells = {
	.boss = {
		.infinity_network = {
			{-1, -1, -1, 1}, AT_SurvivalSpell, "Obliteration “Infinity Network”", 90, 80000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_infinity_network),
			NULL, VIEWPORT_W/2.0+100.0*I, 7,
		},
		.sierpinski = {
			{-1, -1, -1, 2}, AT_Spellcard, "Automaton “Legacy of Sierpiński”", 90, 150000,
			TASK_INDIRECT_INIT(BossAttack, stagex_spell_sierpinski),
			NULL, VIEWPORT_W/2.0+120.0*I, 7,
		},
	},
};

static void stagex_begin(void) {
	stagex_drawsys_init();

	INVOKE_TASK(stagex_animate_background);
	INVOKE_TASK(stagex_timeline);
}

static void stagex_spellpractice_begin(void) {
	stagex_drawsys_init();

	StageXDrawData *draw_data = stagex_get_draw_data();
	draw_data->tower_global_dissolution = 1;
	draw_data->tower_partial_dissolution = 1;

	Boss *boss = stagex_spawn_yumemi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(boss, global.stage->spell, true);
	boss_engage(boss);
	global.boss = boss;

	stage_start_bgm("stagexboss");
}

static void stagex_end(void) {
	stagex_drawsys_shutdown();
}

static void stagex_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"cell_noise",
		"stageex/bg",
		"stageex/bg_binary",
		"stageex/code",
		"stageex/dissolve_mask",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"copy_depth",
		"extra_bg",
		"extra_tower_apply_mask",
		"extra_tower_mask",
		"zbuf_fog",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"tower",
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
