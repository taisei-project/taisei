/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage5.h"
#include "stage5_events.h"

#include "stage.h"
#include "stageutils.h"
#include "global.h"
#include "resource/model.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage5_spells_s stage5_spells = {
	.boss = {
		.atmospheric_discharge = {
			{ 0,  1,  2,  3}, AT_Spellcard, "High Voltage “Atmospheric Discharge”", 60, 44000,
			iku_atmospheric, iku_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.artificial_lightning = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Charge Sign “Artificial Lightning”", 75, 60000,
			iku_lightning, iku_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.induction_field = {
			{12, 13, -1, -1}, AT_Spellcard, "Current Sign “Induction Field”", 60, 50000,
			iku_induction, iku_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.inductive_resonance = {
			{-1, -1, 14, 15}, AT_Spellcard, "Current Sign “Inductive Resonance”", 60, 50000,
			iku_induction, iku_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.natural_cathode = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Spark Sign “Natural Cathode”", 60, 44000,
			iku_cathode, iku_spell_bg, BOSS_DEFAULT_GO_POS
		},
	},

	.extra.overload = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Circuit Sign “Overload”", 60, 44000,
		iku_extra, iku_spell_bg, BOSS_DEFAULT_GO_POS
	},
};

struct {
	float light_strength;

	float rotshift;
	float omega;
	float rad;
} stagedata;

static vec3 **stage5_stairs_pos(vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 0, 6000};

	return linear3dpos(pos, maxrange, p, r);
}

static void stage5_stairs_draw(vec3 pos) {
	r_mat_push();
	r_mat_translate(pos[0], pos[1], pos[2]);
	r_mat_scale(300,300,300);

	r_shader("tower_light");
	r_uniform_sampler("tex", "stage5/tower");
	r_uniform_vec3("lightvec", 0, 0, 0);
	r_uniform_vec4("color", 0.1, 0.1, 0.5, 1);
	r_uniform_float("strength", stagedata.light_strength);

	r_draw_model("tower");

	r_mat_pop();

	r_shader_standard();
}

static void stage5_draw(void) {
	set_perspective(&stage_3d_context, 100, 20000);
	draw_stage3d(&stage_3d_context, 30000);
}

static void stage5_update(void) {
	update_stage3d(&stage_3d_context);

	TIMER(&global.timer);
	float w = 0.005;

	stagedata.rotshift += stagedata.omega;
	stage_3d_context.crot[0] += stagedata.omega*0.5;
	stagedata.rad += stagedata.omega*20;

	int rot_time = 6350;

	FROM_TO(rot_time, rot_time+50, 1)
		stagedata.omega -= 0.005;

	FROM_TO(rot_time+200, rot_time+250, 1)
		stagedata.omega += 0.005;

	stage_3d_context.cx[0] = stagedata.rad*cos(-w*global.frames);
	stage_3d_context.cx[1] = stagedata.rad*sin(-w*global.frames);
	stage_3d_context.cx[2] = -1700+w*3000/M_PI*global.frames;

	stage_3d_context.crot[2] = stagedata.rotshift-180/M_PI*w*global.frames;

	stagedata.light_strength *= 0.98;

	if(frand() < 0.01)
		stagedata.light_strength = 5+5*frand();
}

void iku_spell_bg(Boss *b, int t) {
	fill_viewport(0, 300, 1, "stage5/spell_bg");

	r_blend(BLEND_MOD);
	fill_viewport(0, t*0.001, 0.7, "stage5/noise");
	r_blend(BLEND_PREMUL_ALPHA);

	r_mat_push();
	r_mat_translate(0, -100, 0);

	fill_viewport(t/100.0,0,0.5,"stage5/spell_clouds");
	r_mat_push();
	r_mat_translate(0, 100, 0);
	fill_viewport(t/100.0*0.75,0,0.6,"stage5/spell_clouds");
	r_mat_push();
	r_mat_translate(0, 100, 0);
	fill_viewport(t/100.0*0.5,0,0.7,"stage5/spell_clouds");
	r_mat_push();
	r_mat_translate(0, 100, 0);
	fill_viewport(t/100.0*0.25,0,0.8,"stage5/spell_clouds");
	r_mat_pop();
	r_mat_pop();
	r_mat_pop();
	r_mat_pop();

	float opacity = 0.05*stagedata.light_strength;
	r_color4(opacity, opacity, opacity, opacity);
	fill_viewport(0, 300, 1, "stage5/spell_lightning");
	r_color4(1,1,1,1);
}

static void stage5_start(void) {
	memset(&stagedata, 0, sizeof(stagedata));

	init_stage3d(&stage_3d_context);
	add_model(&stage_3d_context, stage5_stairs_draw, stage5_stairs_pos);

	stage_3d_context.crot[0] = 60;
	stagedata.rotshift = 140;
	stagedata.rad = 2800;
}

static void stage5_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage5", "stage5boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage5/noise",
		"stage5/spell_bg",
		"stage5/spell_clouds",
		"stage5/spell_lightning",
		"stage5/tower",
		"dialog/iku",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"tower_light",
		"lasers/linear",
		"lasers/accelerated",
		"lasers/iku_cathode",
		"lasers/iku_lightning",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/iku",
		"boss/iku_mid",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"tower",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"boom",
		"laser1",
		"enemydeath",
	NULL);
}

static void stage5_end(void) {
	free_stage3d(&stage_3d_context);
}

void stage5_skip(int t) {
	skip_background_anim(stage5_update, t, &global.timer, &global.frames);

	int mskip = global.timer;

	if(mskip > 2900) {
		mskip += 1100;
	}

	audio_backend_music_set_position(mskip / (double)FPS);
}

static void stage5_spellpractice_start(void) {
	stage5_start();
	skip_background_anim(stage5_update, 6960, &global.timer, NULL);

	global.boss = stage5_spawn_iku(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_start_attack(global.boss, global.boss->attacks);

	stage_start_bgm("stage5boss");
}

static void stage5_spellpractice_events(void) {
}

ShaderRule stage5_shaders[] = { NULL };

StageProcs stage5_procs = {
	.begin = stage5_start,
	.preload = stage5_preload,
	.end = stage5_end,
	.draw = stage5_draw,
	.update = stage5_update,
	.event = stage5_events,
	.shader_rules = stage5_shaders,
	.spellpractice_procs = &stage5_spell_procs,
};

StageProcs stage5_spell_procs = {
	.begin = stage5_spellpractice_start,
	.preload = stage5_preload,
	.end = stage5_end,
	.draw = stage5_draw,
	.update = stage5_update,
	.event = stage5_spellpractice_events,
	.shader_rules = stage5_shaders,
};
