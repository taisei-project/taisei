/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stage4.h"

#include "background_anim.h"
#include "draw.h"
#include "i18n/i18n.h"
#include "kurumi.h"
#include "spells/spells.h"
#include "timeline.h"   // IWYU pragma: keep

#include "global.h"
#include "portrait.h"
#include "stage.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage4_spells_s stage4_spells = {
	.mid = {
		.gate_of_walachia = {
			{ 0,  1,  2,  3}, AT_Spellcard, N_("Bloodless “Gate of Walachia”"), 50, 44000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_walachia),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
		.dry_fountain = {
			{ 4,  5, -1, -1}, AT_Spellcard, N_("Bloodless “Dry Fountain”"), 50, 44000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_dryfountain),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
		.red_spike = {
			{-1, -1,  6,  7}, AT_Spellcard, N_("Bloodless “Red Spike”"), 50, 46000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_redspike),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
	},

	.boss = {
		.animate_wall = {
			{ 8,  9, -1, -1}, AT_Spellcard, N_("Limit “Animate Wall”"), 60, 50000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_aniwall),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
		.demon_wall = {
			{-1, -1, 10, 11}, AT_Spellcard, N_("Summoning “Demon Wall”"), 60, 55000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_aniwall),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
		.blow_the_walls = {
			{12, 13, 14, 15}, AT_Spellcard, N_("Power Sign “Blow the Walls”"), 60, 55000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_blowwall),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
		.bloody_danmaku = {
			{18, 19, 16, 17}, AT_Spellcard, N_("Predation “Vampiric Vapor”"), 80, 60000,
			TASK_INDIRECT_INIT(BossAttack, kurumi_vampvape),
			kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
		},
	},

	.extra.vlads_army = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, N_("Blood Magic “Vlad’s Army”"), 60, 50000,
		TASK_INDIRECT_INIT(BossAttack, kurumi_vladsarmy),
		kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4,
	},
};

static void stage4_start(void) {
	stage4_drawsys_init();
	stage4_bg_init_fullstage();
	stage_start_bgm("stage4");
	stage_set_voltage_thresholds(170, 340, 660, 1040);
	INVOKE_TASK(stage4_timeline);
}

static void stage4_spellpractice_start(void) {
	stage4_drawsys_init();
	stage4_bg_init_spellpractice();

	global.boss = stage4_spawn_kurumi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);

	stage_start_bgm("stage4boss");
}

static void stage4_preload(ResourceGroup *rg) {
	portrait_preload_base_sprite(rg, "kurumi", NULL, RESF_DEFAULT);
	portrait_preload_base_sprite(rg, "kurumi", "defeated", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "defeated", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "dissatisfied", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "normal", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "puzzled", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "tsun", RESF_DEFAULT);
	portrait_preload_face_sprite(rg, "kurumi", "tsun_blush", RESF_DEFAULT);
	res_group_preload(rg, RES_BGM, RESF_OPTIONAL, "stage4", "stage4boss", NULL);
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"fractal_noise",
		"stage4/kurumibg2",
	NULL);
	res_group_preload(rg, RES_SPRITE, RESF_DEFAULT,
		"stage4/kurumibg1",
		"stage6/scythe", // Stage 6 is intentional
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"alpha_discard",
		"fireparticles",
		"pbr",
		"sprite_negative",
		"ssr_water",
		"zbuf_fog_tonemap",
	NULL);
	res_group_preload(rg, RES_ANIM, RESF_DEFAULT,
		"boss/kurumi",
	NULL);
	res_group_preload(rg, RES_MATERIAL, RESF_DEFAULT,
		"stage4/corridor",
		"stage4/ground",
		"stage4/mansion",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"stage4/corridor",
		"stage4/ground",
		"stage4/mansion",
	NULL);
	res_group_preload(rg, RES_TEXTURE, RESF_OPTIONAL,
		"part/sinewave",
	NULL);
	res_group_preload(rg, RES_SFX, RESF_OPTIONAL,
		"laser1",
		"boom",
	NULL);

	// XXX: Special case for spell practice of the god damn extra spell, because it always needs a special case.
	// TODO: Maybe add spell-specific preloads instead of putting everything into the stage one?
	enemies_preload(rg);
}

static void stage4_end(void) {
	stage4_drawsys_shutdown();
}

StageProcs stage4_procs = {
	.begin = stage4_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.shader_rules = stage4_bg_effects,
	.spellpractice_procs = &stage4_spell_procs,
};

StageProcs stage4_spell_procs = {
	.begin = stage4_spellpractice_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.shader_rules = stage4_bg_effects,
};
