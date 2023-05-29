/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage3.h"
#include "draw.h"
#include "background_anim.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "timeline.h"

#include "global.h"
#include "portrait.h"
#include "common_tasks.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage3_spells_s stage3_spells = {
	.mid = {
		.deadly_dance = {
			{ 0,  1,  2,  3}, AT_SurvivalSpell, "Disruption “Logic Bomb”", 14, 40000,
			TASK_INDIRECT_INIT(BossAttack, stage3_spell_logic_bomb),
			stage3_draw_scuttle_spellbg, VIEWPORT_W/2.0+100*I, 3,
		},
	},

	.boss = {
		.moonlight_rocket = {
			{ 6,  7,  8,  9}, AT_Spellcard, "Firefly Sign “Moonlight Rocket”", 40, 40000,
			TASK_INDIRECT_INIT(BossAttack, stage3_spell_moonlight_rocket),
			stage3_draw_wriggle_spellbg, VIEWPORT_W/2.0+100*I, 3,
		},
		.wriggle_night_ignite = {
			{10, 11, 12, 13}, AT_Spellcard, "Light Source “Wriggle Night Ignite”", 50, 46000,
			TASK_INDIRECT_INIT(BossAttack, stage3_spell_night_ignite),
			stage3_draw_wriggle_spellbg, VIEWPORT_W/2.0+100*I, 3,
		},
		.firefly_storm = {
			{14, 15, 16, 17}, AT_Spellcard, "Bug Sign “Firefly Storm”", 45, 45000,
			TASK_INDIRECT_INIT(BossAttack, stage3_spell_firefly_storm),
			stage3_draw_wriggle_spellbg, VIEWPORT_W/2.0+100*I, 3,
		},
	},

	.extra.light_singularity = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Lamp Sign “Light Singularity”", 75, 45000,
		TASK_INDIRECT_INIT(BossAttack, stage3_spell_light_singularity),
		stage3_draw_wriggle_spellbg, VIEWPORT_W/2.0+100*I, 3,
	},
};

static void stage3_start(void) {
	stage3_drawsys_init();
	stage3_bg_init_fullstage();
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);
	INVOKE_TASK(stage3_timeline);
}

static void stage3_spellpractice_start(void) {
	stage3_drawsys_init();
	stage3_bg_init_spellpractice();

	if(global.stage->spell->draw_rule == stage3_draw_scuttle_spellbg) {
		global.boss = stage3_spawn_scuttle(BOSS_DEFAULT_SPAWN_POS);
		stage_unlock_bgm("scuttle");
		stage_start_bgm("scuttle");
	} else {
		global.boss = stage3_spawn_wriggle(BOSS_DEFAULT_SPAWN_POS);
		stage_start_bgm("stage3boss");
	}

	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);
}

static void stage3_preload(ResourceGroup *rg) {
	portrait_preload_base_sprite(rg, "wriggle", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "wriggle", "proud", RESF_DEFAULT);
	portrait_preload_base_sprite(rg, "scuttle", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "scuttle", "normal", RESF_DEFAULT);
	res_group_preload(rg, RES_BGM, RESF_OPTIONAL, "stage3", "stage3boss", NULL);
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"ibl_brdf_lut",
		"stage3/envmap",
		"stage3/spellbg1",
		"stage3/spellbg2",
		"stage3/wspellbg",
		"stage3/wspellclouds",
		"stage3/wspellswarm",
	NULL);
	res_group_preload(rg, RES_MATERIAL, RESF_DEFAULT,
		"stage3/ground",
		"stage3/leaves",
		"stage3/rocks",
		"stage3/trees",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"stage3/ground",
		"stage3/leaves",
		"stage3/rocks",
		"stage3/trees",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"glitch",
		"maristar_bombbg",
		"pbr",
		"pbr_roughness_alpha_discard",
		"stage3_wriggle_bg",
		"zbuf_fog",
	NULL);
	res_group_preload(rg, RES_ANIM, RESF_DEFAULT,
		"boss/scuttle",
		"boss/wriggleex",
	NULL);
	res_group_preload(rg, RES_SFX, RESF_OPTIONAL,
		"laser1",
	NULL);
}

static void stage3_end(void) {
	stage3_drawsys_shutdown();
}

StageProcs stage3_procs = {
	.begin = stage3_start,
	.draw = stage3_draw,
	.end = stage3_end,
	.preload = stage3_preload,
	.shader_rules = stage3_bg_effects,
	.postprocess_rules = stage3_postprocess,
	.spellpractice_procs = &(StageProcs) {
		.begin = stage3_spellpractice_start,
		.draw = stage3_draw,
		.end = stage3_end,
		.preload = stage3_preload,
		.shader_rules = stage3_bg_effects,
		.postprocess_rules = stage3_postprocess,
	},
};
