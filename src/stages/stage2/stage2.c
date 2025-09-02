/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stage2.h"

#include "background_anim.h"
#include "draw.h"
#include "hina.h"
#include "intl/intl.h"
#include "spells/spells.h"
#include "stage.h"
#include "timeline.h"   // IWYU pragma: keep

#include "global.h"
#include "portrait.h"

struct stage2_spells_s stage2_spells = {
	.boss = {
		.amulet_of_harm = {
			{ 0,  1,  2,  3}, AT_Spellcard, N_("Shard “Amulet of Harm”"), 50, 30000,
			TASK_INDIRECT_INIT(BossAttack, stage2_spell_amulet_of_harm),
			stage2_draw_hina_spellbg, BOSS_DEFAULT_GO_POS, 2,
		},
		.bad_pick = {
			{ 4,  5,  6,  7}, AT_Spellcard, N_("Lottery Sign “Bad Pick”"), 60, 43200,
			TASK_INDIRECT_INIT(BossAttack, stage2_spell_bad_pick),
			stage2_draw_hina_spellbg, BOSS_DEFAULT_GO_POS, 2,
		},
		.wheel_of_fortune = {
			{ 8,  9, 10, 11}, AT_Spellcard, N_("Lottery Sign “Wheel of Fortune”"), 50, 32000,
			TASK_INDIRECT_INIT(BossAttack, stage2_spell_wheel_of_fortune),
			stage2_draw_hina_spellbg, BOSS_DEFAULT_GO_POS, 2,
		},
	},

	.extra.monty_hall_danmaku = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, N_("Lottery Sign “Monty Hall Danmaku”"), 60, 60000,
		TASK_INDIRECT_INIT(BossAttack, stage2_spell_monty_hall_danmaku),
		stage2_draw_hina_spellbg, BOSS_DEFAULT_GO_POS, 2,
	},
};

static void stage2_start(void) {
	stage2_drawsys_init();
	stage2_bg_init_fullstage();
	stage_start_bgm("stage2");
	stage_set_voltage_thresholds(75, 175, 400, 720);
	INVOKE_TASK(stage2_timeline);
}

static void stage2_spellpractice_start(void) {
	stage2_drawsys_init();
	stage2_bg_init_spellpractice();

	Boss *hina = stage2_spawn_hina(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(hina, global.stage->spell, true);
	boss_engage(hina);
	global.boss = hina;

	stage_start_bgm("stage2boss");
}

static void stage2_end(void) {
	stage2_drawsys_shutdown();
}

static void stage2_preload(ResourceGroup *rg) {
	portrait_preload_base_sprite(rg, "hina", NULL, RESF_DEFAULT);
	portrait_preload_base_sprite(rg, "hina", "defeated", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "hina", "concerned", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "hina", "defeated", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "hina", "normal", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "hina", "serious", RESF_DEFAULT);
	res_group_preload(rg, RES_BGM, RESF_OPTIONAL, "stage2", "stage2boss", NULL);

	res_group_preload(rg, RES_SPRITE, RESF_DEFAULT,
		"fairy_circle_big",
		"part/blast_huge_rays",
		"stage2/spellbg1",
		"stage2/spellbg2",
	NULL);
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"fractal_noise",
		"ibl_brdf_lut",
		"stage2/envmap",
	NULL);
	res_group_preload(rg, RES_MATERIAL, RESF_DEFAULT,
		"stage2/branch",
		"stage2/ground",
		"stage2/lakefloor",
		"stage2/leaves",
		"stage2/rocks",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"stage2/branch",
		"stage2/grass",
		"stage2/ground",
		"stage2/leaves",
		"stage2/rocks",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"fireparticles",
		"pbr",
		"pbr_diffuse_alpha_discard",
		"pbr_water",
		"zbuf_fog_tonemap",
	NULL);
	res_group_preload(rg, RES_ANIM, RESF_DEFAULT,
		"boss/wriggle",
		"boss/hina",
		"fire",
	NULL);
	res_group_preload(rg, RES_SFX, RESF_OPTIONAL,
		"laser1",
	NULL);
}

StageProcs stage2_procs = {
	.begin = stage2_start,
	.preload = stage2_preload,
	.end = stage2_end,
	.draw = stage2_draw,
	.shader_rules = stage2_bg_effects,
	.spellpractice_procs = &(StageProcs) {
		.begin = stage2_spellpractice_start,
		.preload = stage2_preload,
		.end = stage2_end,
		.draw = stage2_draw,
		.shader_rules = stage2_bg_effects,
	},
};
