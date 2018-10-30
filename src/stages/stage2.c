/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage2.h"
#include "stage2_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage2_spells_s stage2_spells = {
	.boss = {
		.amulet_of_harm = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Shard “Amulet of Harm”", 26, 50000,
			hina_amulet, hina_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.bad_pick = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Lottery Sign “Bad Pick”", 30, 43200,
			hina_bad_pick, hina_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.wheel_of_fortune_easy = {
			{ 8,  9, -1, -1}, AT_Spellcard, "Lottery Sign “Wheel of Fortune”", 20, 36000,
			hina_wheel, hina_spell_bg, BOSS_DEFAULT_GO_POS
		},
		.wheel_of_fortune_hard = {
			{-1, -1, 10, 11}, AT_Spellcard, "Lottery Sign “Wheel of Fortune”", 25, 36000,
			hina_wheel, hina_spell_bg, BOSS_DEFAULT_GO_POS
		},
	},

	.extra.monty_hall_danmaku = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Lottery Sign “Monty Hall Danmaku”", 60, 60000,
		hina_monty, hina_spell_bg, BOSS_DEFAULT_GO_POS
	},
};

static void stage2_bg_leaves_draw(vec3 pos) {
	r_shader("alpha_depth");

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_scale(-1,1,1);
	r_mat_mode(MM_MODELVIEW);

	r_uniform_sampler("tex", "stage2/leaves");

	r_mat_push();
	r_mat_translate(pos[0]-360,pos[1],pos[2]+500);
	r_mat_rotate_deg(-160,0,1,0);
	r_mat_translate(-50,0,0);
	r_mat_scale(1000,3000,1);

	r_draw_quad();

	r_mat_pop();

	r_shader_standard();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);
}

static void stage2_bg_grass_draw(vec3 pos) {
	r_disable(RCAP_DEPTH_TEST);
	r_uniform_sampler("tex", "stage2/roadgrass");

	r_mat_push();
	r_mat_translate(pos[0]+250,pos[1],pos[2]+40);
	r_mat_rotate_deg(pos[2]/2-14,0,1,0);

	r_mat_scale(-500,2000,1);
	r_draw_quad();
	r_mat_pop();

	r_enable(RCAP_DEPTH_TEST);
}

static void stage2_bg_ground_draw(vec3 pos) {
	r_mat_push();
	r_mat_translate(pos[0]-50,pos[1],pos[2]);
	r_mat_scale(-1000,1000,1);

	r_color4(0.08,0.,0.1,1);
	r_shader_standard_notex();
	r_draw_quad();
	r_shader_standard();
	r_uniform_sampler("tex", "stage2/roadstones");
	r_color4(0.5,0.5,0.5,1);
	r_draw_quad();
	r_color4(1,1,1,1);
	r_mat_translate(0,0,+10);
	r_draw_quad();

	r_mat_pop();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_translate(global.frames/100.0,1*sin(global.frames/100.0),0);
	r_mat_mode(MM_MODELVIEW);

	r_mat_push();

	r_uniform_sampler("tex", "stage2/border");

	r_mat_translate(pos[0]+410,pos[1],pos[2]+600);
	r_mat_rotate_deg(90,0,1,0);
	r_mat_scale(1200,1000,1);

	r_draw_quad();

	r_mat_pop();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);
}

static vec3 **stage2_bg_pos(vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 1000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

static vec3 **stage2_bg_grass_pos(vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 2000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

static vec3 **stage2_bg_grass_pos2(vec3 pos, float maxrange) {
	vec3 p = {0, 1234, 40};
	vec3 r = {0, 2000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

static void stage2_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 0.05, 0.0, 0.03, 1.0);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage2_bloom(Framebuffer *fb) {
	r_shader("bloom");
	r_uniform_int("samples", 10);
	r_uniform_float("intensity", 0.05);
	r_uniform_float("radius", 0.03);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage2_start(void) {
	init_stage3d(&stage_3d_context);
	stage_3d_context.cx[2] = 1000;
	stage_3d_context.cx[0] = -850;
	stage_3d_context.crot[0] = 60;
	stage_3d_context.crot[2] = -90;

	stage_3d_context.cv[0] = 9;

	add_model(&stage_3d_context, stage2_bg_ground_draw, stage2_bg_pos);
	add_model(&stage_3d_context, stage2_bg_grass_draw, stage2_bg_grass_pos);
	add_model(&stage_3d_context, stage2_bg_grass_draw, stage2_bg_grass_pos2);
	add_model(&stage_3d_context, stage2_bg_leaves_draw, stage2_bg_pos);
}

static void stage2_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage2", "stage2boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage2/border",
		"stage2/leaves",
		"stage2/roadgrass",
		"stage2/roadstones",
		"stage2/spellbg1",
		"stage2/spellbg2",
		"dialog/hina",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"bloom",
		"zbuf_fog",
		"alpha_depth",
		"lasers/linear",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/wriggle",
		"boss/hina",
		"fire",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
	NULL);
}

static void stage2_end(void) {
	free_stage3d(&stage_3d_context);
}

static void stage2_draw(void) {
	set_perspective(&stage_3d_context, 500, 5000);
	draw_stage3d(&stage_3d_context, 7000);
}

static void stage2_update(void) {
	TIMER(&global.frames);

	FROM_TO(0, 180, 1) {
		stage_3d_context.cv[0] = approach(stage_3d_context.cv[0], 0, 0.05);
		stage_3d_context.cv[1] = approach(stage_3d_context.cv[1], 9, 0.05);
	}

	stage_3d_context.crot[2] += min(0.5, -stage_3d_context.crot[2] * 0.02);

	update_stage3d(&stage_3d_context);
}

static void stage2_spellpractice_start(void) {
	stage2_start();
	skip_background_anim(stage2_update, 180, &global.frames, NULL);

	Boss* hina = stage2_spawn_hina(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(hina, global.stage->spell, true);
	boss_start_attack(hina, hina->attacks);
	global.boss = hina;

	stage_start_bgm("stage2boss");
}

static void stage2_spellpractice_events(void) {
}

ShaderRule stage2_shaders[] = { stage2_fog, stage2_bloom, NULL };

StageProcs stage2_procs = {
	.begin = stage2_start,
	.preload = stage2_preload,
	.end = stage2_end,
	.draw = stage2_draw,
	.update = stage2_update,
	.event = stage2_events,
	.shader_rules = stage2_shaders,
	.spellpractice_procs = &stage2_spell_procs,
};

StageProcs stage2_spell_procs = {
	.begin = stage2_spellpractice_start,
	.preload = stage2_preload,
	.end = stage2_end,
	.draw = stage2_draw,
	.update = stage2_update,
	.event = stage2_spellpractice_events,
	.shader_rules = stage2_shaders,
};
