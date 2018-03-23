/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage1.h"
#include "stage1_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stagedraw.h"
#include "resource/model.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage1_spells_s stage1_spells = {
	.mid = {
		.perfect_freeze = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Freeze Sign ~ Perfect Freeze", 32, 24000,
			cirno_perfect_freeze, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
		},
	},

	.boss = {
		.crystal_rain = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Freeze Sign ~ Crystal Rain", 28, 33000,
			cirno_crystal_rain, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
		},
		.snow_halation = {
			{-1, -1, 12, 13}, AT_Spellcard, "Winter Sign ~ Snow Halation", 40, 40000,
			cirno_snow_halation, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
		},
		.icicle_fall = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Doom Sign ~ Icicle Fall", 35, 40000,
			cirno_icicle_fall, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
		},
	},

	.extra.crystal_blizzard = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Frost Sign ~ Crystal Blizzard", 60, 40000,
		cirno_crystal_blizzard, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
	},
};

#ifdef SPELL_BENCHMARK
AttackInfo stage1_spell_benchmark = {
	{-1, -1, -1, -1, 127}, AT_SurvivalSpell, "Profiling ~ ベンチマーク", 40, 40000,
	cirno_benchmark, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
};
#endif

static bool particle_filter(Projectile *part) {
	return !(part->flags & PFLAG_NOREFLECT) && stage_should_draw_particle(part);
}

static void stage1_bg_draw(vec3 pos) {
	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);

	r_mat_push();
	r_mat_translate(0,stage_3d_context.cx[1]+500,0);
	r_mat_rotate_deg(180,1,0,0);

	r_shader_standard_notex();
	r_mat_push();
	r_mat_scale(1200,3000,1);
	r_color4(0,0.1,.1,1);
	r_draw_quad();
	r_color4(1,1,1,1);
	r_mat_pop();
	r_shader_standard();

	r_mat_push();
	r_mat_rotate_deg(30,1,0,0);
	r_mat_scale(.85,-.85,.85);
	r_mat_translate(-VIEWPORT_W/2,0,0);
	r_disable(RCAP_CULL_FACE);
	r_disable(RCAP_DEPTH_TEST);
	draw_projectiles(global.particles, particle_filter);
	draw_enemies(global.enemies);

	if(global.boss) {
		draw_boss(global.boss);
	}

	r_mat_pop();
	r_capability(RCAP_CULL_FACE, cullcap_saved);

	r_shader_standard_notex();
	r_mat_push();
	r_mat_scale(1200,3000,1);
	r_color4(0,0.1,.1,0.8);
	r_draw_quad();
	r_color4(1,1,1,1);
	r_mat_pop();
	r_enable(RCAP_DEPTH_TEST);
	r_mat_pop();
	r_shader_standard();
}

static vec3 **stage1_bg_pos(vec3 p, float maxrange) {
	vec3 q = {0,0,0};
	return single3dpos(p, INFINITY, q);
}

static void stage1_smoke_draw(vec3 pos) {
	float d = fabsf(pos[1]-stage_3d_context.cx[1]);

	r_disable(RCAP_DEPTH_TEST);
	r_mat_push();
	r_mat_translate(pos[0]+200*sin(pos[1]), pos[1], pos[2]+200*sin(pos[1]/25.0));
	r_mat_rotate_deg(90,-1,0,0);
	r_mat_scale(3.5,2,1);
	r_mat_rotate_deg(global.frames,0,0,1);

	r_color4(.8,.8,.8,((d-500)*(d-500))/1.5e7);
	draw_sprite(0,0,"stage1/fog");
	r_color4(1,1,1,1);

	r_mat_pop();
	r_enable(RCAP_DEPTH_TEST);
}

static vec3 **stage1_smoke_pos(vec3 p, float maxrange) {
	vec3 q = {0,0,-300};
	vec3 r = {0,300,0};
	return linear3dpos(p, maxrange/2.0, q, r);
}

static void stage1_fog(FBO *fbo) {
	r_shader("zbuf_fog");
	r_uniform_int("tex", 0);
	r_uniform_int("depth", 1);
	r_uniform_vec4("fog_color", 0.8, 0.8, 0.8, 1.0);
	r_uniform_float("start", 0.0);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0.2);
	r_texture_ptr(1, r_target_get_attachment(fbo, RENDERTARGET_ATTACHMENT_DEPTH));
	draw_fbo_viewport(fbo);
	r_shader_standard();
}

static void stage1_draw(void) {
	set_perspective(&stage_3d_context, 500, 5000);
	draw_stage3d(&stage_3d_context, 7000);
}

static void stage1_update(void) {
	update_stage3d(&stage_3d_context);
}

static void stage1_reed_draw(vec3 pos) {
	bool depthwrite_saved = r_capability_current(RCAP_DEPTH_WRITE);
	DepthTestFunc depthfunc_saved = r_depth_func_current();

	float d = -55+50*sin(pos[1]/25.0);
	r_mat_push();
	r_mat_translate(pos[0]+200*sin(pos[1]), pos[1], d);
	r_mat_rotate_deg(90,1,0,0);
	r_mat_scale(80,80,80);
	r_color4(0.,0.05,0.05,1);
	r_draw_model("reeds");
	r_mat_translate(0,-d/80,0);
	r_mat_scale(1,-1,1);
	r_mat_translate(0,d/80,0);
	r_depth_func(DEPTH_GREATER);
	r_disable(RCAP_DEPTH_WRITE);
	r_color4(0.,0.05,0.05,0.5);
	r_draw_model("reeds");
	r_color4(1,1,1,1);
	r_mat_pop();

	r_capability(RCAP_DEPTH_WRITE, depthwrite_saved);
	r_depth_func(depthfunc_saved);
}

static void stage1_start(void) {
	init_stage3d(&stage_3d_context);
	add_model(&stage_3d_context, stage1_bg_draw, stage1_bg_pos);
	add_model(&stage_3d_context, stage1_smoke_draw, stage1_smoke_pos);
	add_model(&stage_3d_context, stage1_reed_draw, stage1_smoke_pos);

	stage_3d_context.crot[0] = 60;
	stage_3d_context.cx[2] = 700;
	stage_3d_context.cv[1] = 4;
}

static void stage1_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage1", "stage1boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage1/cirnobg",
		"stage1/fog",
		"stage1/snowlayer",
		"dialog/cirno",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"reeds",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"zbuf_fog",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/cirno",
	NULL);
}

static void stage1_end(void) {
	free_stage3d(&stage_3d_context);
}

static void stage1_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		Boss* cirno = stage1_spawn_cirno(BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(cirno, global.stage->spell, true);
		boss_start_attack(cirno, cirno->attacks);
		global.boss = cirno;

		stage_start_bgm("stage1boss");
	}
}

ShaderRule stage1_shaders[] = { stage1_fog, NULL };

StageProcs stage1_procs = {
	.begin = stage1_start,
	.preload = stage1_preload,
	.end = stage1_end,
	.draw = stage1_draw,
	.update = stage1_update,
	.event = stage1_events,
	.shader_rules = stage1_shaders,
	.spellpractice_procs = &stage1_spell_procs,
};

StageProcs stage1_spell_procs = {
	.preload = stage1_preload,
	.begin = stage1_start,
	.end = stage1_end,
	.draw = stage1_draw,
	.update = stage1_update,
	.event = stage1_spellpractice_events,
	.shader_rules = stage1_shaders,
};
