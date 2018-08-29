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

static FBPair stage1_bg_fbpair;

#ifdef SPELL_BENCHMARK
AttackInfo stage1_spell_benchmark = {
	{-1, -1, -1, -1, 127}, AT_SurvivalSpell, "Profiling ~ ベンチマーク", 40, 40000,
	cirno_benchmark, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
};
#endif

static bool particle_filter(Projectile *part) {
	return !(part->flags & PFLAG_NOREFLECT) && stage_should_draw_particle(part);
}

static bool stage1_draw_predicate(EntityInterface *ent) {
	if(ent->draw_layer == LAYER_PLAYER_SLAVE || ent->draw_layer == LAYER_PLAYER_FOCUS) {
		return false;
	}

	if(ent->type == ENT_BOSS) {
		return true;
	}

	if(ent->type == ENT_ENEMY) {
		Enemy *e = ENT_CAST(ent, Enemy);

		if(e->hp == ENEMY_BOMB) {
			return false;
		}

		return true;
	}

	if(ent->type == ENT_PROJECTILE) {
		Projectile *p = ENT_CAST(ent, Projectile);

		if(p->type == Particle) {
			return particle_filter(p);
		}
	}

	return false;
}

static void stage1_water_draw(vec3 pos) {
	// don't even ask

	r_state_push();

	r_mat_push();
	r_mat_translate(0,stage_3d_context.cx[1]+500,0);
	r_mat_rotate_deg(180,1,0,0);

	r_shader_standard_notex();
	r_mat_push();
	r_mat_scale(1200,3000,1);
	r_color4(0, 0.1, 0.1, 1);
	r_draw_quad();
	r_color4(1, 1, 1, 1);
	r_mat_pop();

	r_disable(RCAP_CULL_FACE);
	r_disable(RCAP_DEPTH_TEST);

	Framebuffer *bg_fb = r_framebuffer_current();
	FBPair *fbpair = &stage1_bg_fbpair;
	r_framebuffer(fbpair->back);
	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_identity();
	r_mat_push();

	float z = 0.75;
	float zo = -0.05;

	r_mat_translate(VIEWPORT_W * 0.5 * (1 - z), VIEWPORT_H * 0.5 * (1 - z), 0);
	r_mat_scale(z, z, 1);
	r_clear_color4(0, 0.08, 0.08, 1);
	r_clear(CLEAR_ALL);
	r_shader("sprite_default");

	ent_draw(stage1_draw_predicate);

	r_mat_push();
	r_shader_standard_notex();
	r_mat_translate(VIEWPORT_W*0.5, VIEWPORT_H*0.5, 0);
	r_mat_scale(VIEWPORT_W/z, VIEWPORT_H/z, 1);
	r_color4(0, 0.08, 0.08, 0.8);
	r_draw_quad();
	r_mat_pop();

	r_mat_pop();

	fbpair_swap(fbpair);
	r_framebuffer(fbpair->back);
	r_shader("blur5");
	r_uniform_vec2("blur_resolution", VIEWPORT_W, VIEWPORT_H);
	r_uniform_vec2("blur_direction", 1, 0);
	draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(fbpair);
	r_framebuffer(fbpair->back);
	r_uniform_vec2("blur_direction", 0, 1);
	draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(fbpair);

	r_mat_pop();
	r_mat_mode(MM_PROJECTION);
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_framebuffer(bg_fb);

	r_shader_standard();

	r_enable(RCAP_DEPTH_TEST);
	r_disable(RCAP_DEPTH_WRITE);

	r_shader("stage1_water");
	r_uniform_float("time", 0.5 * global.frames / (float)FPS);
	r_texture_ptr(1, r_framebuffer_get_attachment(bg_fb, FRAMEBUFFER_ATTACH_DEPTH));

	r_mat_push();
	r_mat_translate(0, 70, 0);
	r_mat_rotate_deg(10,1,0,0);
	r_mat_scale(.85/(z+zo),-.85/(z+zo),.85);
	r_mat_translate(-VIEWPORT_W/2,0,0);
	draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
	r_mat_pop();

	r_shader_standard_notex();
	r_mat_push();
	r_mat_scale(1200,3000,1);
	r_color4(0, 0.08, 0.08, 0.08);
	r_draw_quad();
	r_mat_pop();

	r_mat_pop();
	r_state_pop();
}

static vec3 **stage1_bg_pos(vec3 p, float maxrange) {
	vec3 q = {0,0,0};
	return single3dpos(p, INFINITY, q);
}

static void stage1_smoke_draw(vec3 pos) {
	float d = fabsf(pos[1]-stage_3d_context.cx[1]);

	r_state_push();
	r_shader("sprite_default");
	r_disable(RCAP_DEPTH_TEST);
	r_cull(CULL_BACK);
	r_mat_push();
	r_mat_translate(pos[0]+200*sin(pos[1]), pos[1], pos[2]+200*sin(pos[1]/25.0));
	r_mat_rotate_deg(90,-1,0,0);
	r_mat_scale(3.5,2,1);
	r_mat_rotate_deg(global.frames,0,0,1);
	float o = ((d-500)*(d-500))/1.5e7;
	r_draw_sprite(&(SpriteParams) {
		.sprite = "stage1/fog",
		.color = RGBA(0.8 * o, 0.8 * o, 0.8 * o, o * 0.5),
	});
	r_mat_pop();
	r_state_pop();
}

static vec3 **stage1_smoke_pos(vec3 p, float maxrange) {
	vec3 q = {0,0,-300};
	vec3 r = {0,300,0};
	return linear3dpos(p, maxrange/2.0, q, r);
}

static void stage1_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_int("tex", 0);
	r_uniform_int("depth", 1);
	r_uniform_vec4("fog_color", 0.8, 0.8, 0.8, 1.0);
	r_uniform_float("start", 0.0);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0.2);
	r_texture_ptr(1, r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage1_draw(void) {
	set_perspective(&stage_3d_context, 500, 5000);
	draw_stage3d(&stage_3d_context, 7000);
}

static void stage1_update(void) {
	update_stage3d(&stage_3d_context);
}

static inline uint32_t floathash(float f) {
	return (union { uint32_t i; float f; }){ .f = f }.i;
}

static void stage1_waterplants_draw(vec3 pos) {
	r_state_push();

	// stateless pseudo-random fuckery
	int tile = floathash(pos[1] * 3124312) & 1;
	float offs = 200 * sin(2*M_PI*remainder(3214.322211333 * floathash(pos[1]), M_E));
	float d = -55+50*sin(pos[1]/25.0);
	r_mat_push();
	r_mat_translate(pos[0]+offs, pos[1], d);
	r_mat_rotate(2*M_PI*sin(32.234*pos[1]) + global.frames / 800.0, 0, 0, 1);

	Sprite spr = { 0 };
	spr.w = spr.h = 1;
	spr.tex = get_tex("stage1/waterplants");
	spr.tex_area.w = spr.tex->w * 0.5;
	spr.tex_area.h = spr.tex->h;
	spr.tex_area.x = spr.tex_area.w * tile;

	float a = 0.8;
	float s = 160 * (1 + 2 * psin(13.12993*pos[1]));
	r_mat_scale(s, s, 1);
	r_depth_func(DEPTH_GREATER);
	r_disable(RCAP_DEPTH_WRITE);
	r_cull(CULL_FRONT);
	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_scale(0.5, 1, 1);
	r_mat_translate(tile, 0, 0);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = &spr,
		.shader = "sprite_default",
		.flip.x = floathash(pos[1] * 231544.213) & 1,
		.flip.y = floathash(pos[1] * 941233.513) & 1,
		.color = RGBA(0.5*a, 0.4*a, 0.5*a, 0.5*a),
	});
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_mat_pop();
	r_state_pop();
}

static void stage1_start(void) {
	init_stage3d(&stage_3d_context);
	add_model(&stage_3d_context, stage1_water_draw, stage1_bg_pos);
	add_model(&stage_3d_context, stage1_waterplants_draw, stage1_smoke_pos);
	add_model(&stage_3d_context, stage1_smoke_draw, stage1_smoke_pos);

	stage_3d_context.crot[0] = 60;
	stage_3d_context.cx[2] = 700;
	stage_3d_context.cv[1] = 4;

	FBAttachmentConfig cfg = { 0 };
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.type = TEX_TYPE_RGB;
	cfg.tex_params.wrap.s = TEX_WRAP_CLAMP;
	cfg.tex_params.wrap.t = TEX_WRAP_CLAMP;

	stage1_bg_fbpair.front = stage_add_background_framebuffer(0.5, 1, &cfg);
	stage1_bg_fbpair.back = stage_add_background_framebuffer(0.5, 1, &cfg);
}

static void stage1_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage1", "stage1boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage1/cirnobg",
		"stage1/fog",
		"stage1/snowlayer",
		"stage1/waterplants",
		"dialog/cirno",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"zbuf_fog",
		"lasers/linear",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/cirno",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
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
