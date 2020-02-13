/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage4.h"
#include "stage4_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "util/glm.h"
#include "resource/model.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage4_spells_s stage4_spells = {
	.mid = {
		.gate_of_walachia = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Bloodless “Gate of Walachia”", 50, 44000,
			kurumi_slaveburst, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.dry_fountain = {
			{ 4,  5, -1, -1}, AT_Spellcard, "Bloodless “Dry Fountain”", 50, 44000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.red_spike = {
			{-1, -1,  6,  7}, AT_Spellcard, "Bloodless “Red Spike”", 50, 46000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.boss = {
		.animate_wall = {
			{ 8,  9, -1, -1}, AT_Spellcard, "Limit “Animate Wall”", 60, 50000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.demon_wall = {
			{-1, -1, 10, 11}, AT_Spellcard, "Summoning “Demon Wall”", 60, 55000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.blow_the_walls = {
			{12, 13, 14, 15}, AT_Spellcard, "Power Sign “Blow the Walls”", 60, 55000,
			kurumi_blowwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.bloody_danmaku = {
			{18, 19, 16, 17}, AT_Spellcard, "Predation “Vampiric Vapor”", 80, 60000,
			kurumi_danmaku, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.extra.vlads_army = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Blood Magic “Vlad’s Army”", 60, 50000,
		kurumi_extra, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
	},
};

static bool stage4_fog(Framebuffer *fb) {
	float f = 0;
	int redtime = 5100 + STAGE4_MIDBOSS_MUSIC_TIME;

	if(global.timer > redtime) {
		float v = (global.timer-redtime)*0.0005;
		f =  v < 0.1 ? v : 0.1;
	}

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 10.0*f, 0.0, 0.1-f, 1.0);
	r_uniform_float("start", 0.4);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", 4.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	return true;
}

static uint stage4_fountain_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 400, 1500};
	vec3 r = {0, 0, 3000};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][2] > 0) {
			s3d->pos_buffer[i][2] = -9000;
		}
	}

	return num;
}

static void stage4_fountain_draw(vec3 pos) {
	r_uniform_sampler("tex", "stage2/border");
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_rotate(-M_PI/2, 1, 0, 0);
	r_mat_mv_scale(1000, 3010, 1);
	r_draw_quad();
	r_mat_mv_pop();
}

static uint stage4_lake_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 600, 0};
	return single3dpos(s3d, pos, maxrange, p);
}

static void stage4_lake_draw(vec3 pos) {
	r_uniform_sampler("tex", "stage4/lake");
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1]+140, pos[2]);
	r_mat_mv_scale(15, 15, 15);
	r_draw_model("lake");
	r_mat_mv_pop();

	r_uniform_sampler("tex", "stage4/mansion");
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1] + 944, pos[2] + 50);
	r_mat_mv_scale(30, 30, 30);
	r_draw_model("mansion");
	r_mat_mv_pop();
}

static uint stage4_corridor_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 2400, 50};
	vec3 r = {0, 2000, 0};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][1] < p[1]) {
			s3d->pos_buffer[i][1] = -9000;
		}
	}

	return num;
}

static void stage4_corridor_draw(vec3 pos) {
	r_state_push();
	r_uniform_sampler("tex", "stage4/planks");

	r_mat_tex_push();
	r_mat_tex_scale(1, 2, 1);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_mat_mv_push();
	r_mat_mv_rotate(M_PI, 1,0,0);
	r_mat_mv_scale(300, 2000, 1);

	r_draw_quad();
	r_mat_mv_pop();

	r_uniform_sampler("tex", "stage4/wall");

	r_mat_tex_pop();
	r_mat_tex_push();
	r_mat_tex_rotate(M_PI/2, 0, 0, 1);
	r_mat_tex_scale(1, 10, 1);

	r_mat_mv_push();
	r_mat_mv_translate(100, 5, 75);
	r_mat_mv_rotate(M_PI/2, 0, 1, 0);
	r_mat_mv_scale(150, 2000, 1);
	r_draw_quad();
	r_mat_mv_pop();

	r_mat_mv_push();
	r_mat_mv_translate(-100, 5, 75);
	r_mat_mv_rotate(M_PI, 1, 0, 0);
	r_mat_mv_rotate(-M_PI/2, 0, 1, 0);
	r_mat_mv_scale(150, 2000, 1);
	r_draw_quad();
	r_mat_mv_pop();

	r_mat_tex_pop();

	r_shader_standard_notex();

	r_color3(0.01,0.01,0.01);
	r_mat_mv_push();
	r_mat_mv_translate(0,0,150);
	r_mat_mv_scale(500,2000,1);
	r_draw_quad();
	r_mat_mv_pop();

	r_mat_mv_pop();
	r_state_pop();
}

static void stage4_start(void) {
	stage3d_init(&stage_3d_context, 16);

	stage_3d_context.cx[2] = -10000;
	stage_3d_context.cv[2] = 19.7;
	stage_3d_context.crot[0] = 80;

	// for testing
//  stage_3d_context.cx[1] = 2000;
//  stage_3d_context.cx[2] = 130;
//  stage_3d_context.cv[1] = 10;
//  stage_3d_context.crot[0] = 80;
}

static void stage4_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage4", "stage4boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage2/border", // Stage 2 is intentional!
		"stage4/kurumibg1",
		"stage4/kurumibg2",
		"stage4/lake",
		"stage4/mansion",
		"stage4/planks",
		"stage4/wall",
		"dialog/kurumi",
	NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage6/scythe", // Stage 6 is also intentional
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"zbuf_fog",
		"sprite_negative",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/kurumi",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"mansion",
		"lake",
	NULL);
	preload_resources(RES_TEXTURE, RESF_OPTIONAL,
		"part/sinewave",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
		"boom",
		"warp",
	NULL);

	// XXX: Special case for spell practice of the god damn extra spell, because it always needs a special case.
	// TODO: Maybe add spell-specific preloads instead of putting everything into the stage one?
	enemies_preload();
}

static void stage4_end(void) {
	stage3d_shutdown(&stage_3d_context);
}

static void stage4_draw(void) {
	stage3d_set_perspective(&stage_3d_context, 130, 3000);

	Stage3DSegment segs[] = {
		{ stage4_lake_draw, stage4_lake_pos },
		{ stage4_fountain_draw, stage4_fountain_pos },
		{ stage4_corridor_draw, stage4_corridor_pos },
	};

	stage3d_draw(&stage_3d_context, 4000, ARRAY_SIZE(segs), segs);
}

static void stage4_update(void) {
	stage3d_update(&stage_3d_context);

	if(stage_3d_context.cx[2] >= -1000 && stage_3d_context.cv[2] > 0)
		stage_3d_context.cv[2] -= 0.17;

	if(stage_3d_context.cx[1] < 100 && stage_3d_context.cv[2] < 0)
		stage_3d_context.cv[2] = 0;

	if(stage_3d_context.cx[2] >= 0 && stage_3d_context.cx[2] <= 10)
		stage_3d_context.cv[1] += 0.2;

	if(stage_3d_context.cx[1] >= 1200 && stage_3d_context.cx[1] <= 2000)
		stage_3d_context.cv[1] += 0.02;
}

static void stage4_spellpractice_start(void) {
	stage4_start();
	skip_background_anim(stage4_update, 3200, &global.frames, NULL);

	global.boss = stage4_spawn_kurumi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_start_attack(global.boss, global.boss->attacks);

	stage_start_bgm("stage4boss");
}

static void stage4_spellpractice_events(void) {
}

void stage4_skip(int t) {
	skip_background_anim(stage4_update, t, &global.timer, &global.frames);
	audio_music_set_position(global.timer / (double)FPS);
}

ShaderRule stage4_shaders[] = { stage4_fog, NULL };

StageProcs stage4_procs = {
	.begin = stage4_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.update = stage4_update,
	.event = stage4_events,
	.shader_rules = stage4_shaders,
	.spellpractice_procs = &stage4_spell_procs,
};

StageProcs stage4_spell_procs = {
	.begin = stage4_spellpractice_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.update = stage4_update,
	.event = stage4_spellpractice_events,
	.shader_rules = stage4_shaders,
};
