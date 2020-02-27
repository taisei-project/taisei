/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage1.h"
#include "stage1_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stagedraw.h"
#include "resource/model.h"
#include "util/glm.h"
#include "common_tasks.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage1_spells_s stage1_spells = {
	.mid = {
		.perfect_freeze = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Freeze Sign “Perfect Freeze”", 50, 24000,
			NULL, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I, 1, TASK_INDIRECT_INIT(BossAttack, stage1_spell_perfect_freeze)
		},
	},

	.boss = {
		.crystal_rain = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Freeze Sign “Crystal Rain”", 40, 33000,
			NULL, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I, 1, TASK_INDIRECT_INIT(BossAttack, stage1_spell_crystal_rain)
		},
		.snow_halation = {
			{-1, -1, 12, 13}, AT_Spellcard, "Winter Sign “Snow Halation”", 50, 40000,
			NULL, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I, 1, TASK_INDIRECT_INIT(BossAttack, stage1_spell_snow_halation)
		},
		.icicle_cascade = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Doom Sign “Icicle Cascade”", 40, 40000,
			NULL, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I, 1,
			TASK_INDIRECT_INIT(BossAttack, stage1_spell_icicle_cascade)
		},
	},

	.extra.crystal_blizzard = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Frost Sign “Crystal Blizzard”", 60, 40000,
		cirno_crystal_blizzard, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I, 1
	},
};

static struct {
	FBPair water_fbpair;

	struct {
		float near, near_target;
		float far, far_target;
	} fog;

	struct {
		float opacity, opacity_target;
	} snow;

	float pitch_target;
} stage1_bg;

#ifdef SPELL_BENCHMARK
AttackInfo stage1_spell_benchmark = {
	{-1, -1, -1, -1, 127}, AT_SurvivalSpell, "Profiling “ベンチマーク”", 40, 40000,
	cirno_benchmark, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I
};
#endif

static bool particle_filter(Projectile *part) {
	return !(part->flags & PFLAG_NOREFLECT) && stage_should_draw_particle(part);
}

static bool stage1_draw_predicate(EntityInterface *ent) {
	switch(ent->draw_layer & ~LAYER_LOW_MASK) {
		case LAYER_PLAYER_SLAVE:
		case LAYER_PLAYER_FOCUS:
		case LAYER_PLAYER_SHOT:
		case LAYER_PLAYER_SHOT_HIGH:
			return false;
		default: break;
	}

	switch(ent->type) {
		case ENT_BOSS: return true;
		case ENT_ENEMY: {
			Enemy *e = ENT_CAST(ent, Enemy);

			if(e->hp == ENEMY_BOMB) {
				return false;
			}

			return true;
		}
		case ENT_PROJECTILE: {
			Projectile *p = ENT_CAST(ent, Projectile);

			if(p->type == PROJ_PARTICLE) {
				return particle_filter(p);
			}

			return false;
		}
		default: return false;
	}

	UNREACHABLE;
}

static void stage1_water_draw(vec3 pos) {
	// don't even ask

	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate(0, stage_3d_context.cx[1] + 500, 0);
	r_mat_mv_rotate(M_PI, 1, 0, 0);

	static const Color water_color = { 0, 0.08, 0.08, 1 };

	r_clear(CLEAR_COLOR, &water_color, 1);

	r_mat_mv_push();
	r_mat_mv_translate(0, -9000, 0);
	r_mat_mv_rotate(M_PI/2, 1, 0, 0);
	r_mat_mv_scale(3640*1.4, 1456*1.4, 1);
	r_mat_mv_translate(0, -0.5, 0);
	r_shader_standard();
	r_uniform_sampler("tex", "stage1/horizon");
	r_draw_quad();
	r_mat_mv_pop();

	r_state_push();
	r_enable(RCAP_DEPTH_TEST);
	r_depth_func(DEPTH_ALWAYS);
	r_shader_standard_notex();
	r_mat_mv_push();
	r_mat_mv_scale(80000, 80000, 1);
	r_color(&water_color);
	r_draw_quad();
	r_color4(1, 1, 1, 1);
	r_mat_mv_pop();
	r_state_pop();

	r_disable(RCAP_DEPTH_TEST);
	r_disable(RCAP_CULL_FACE);

	Framebuffer *bg_fb = r_framebuffer_current();
	FBPair *fbpair = &stage1_bg.water_fbpair;
	r_framebuffer(fbpair->back);
	r_mat_proj_push();
	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_mat_mv_push_identity();

	float hack = (stage_3d_context.crot[0] - 60) / 15.0;

	float z = glm_lerp(0.75, 0.8, hack);
	float zo = glm_lerp(-0.05, -0.3, hack);

	r_mat_mv_translate(VIEWPORT_W * 0.5 * (1 - z), VIEWPORT_H * 0.5 * (1 - z), 0);
	r_mat_mv_scale(z, z, 1);
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 0), 1);
	r_shader("sprite_default");

	ent_draw(stage1_draw_predicate);

	r_mat_mv_pop();

	fbpair_swap(fbpair);
	r_framebuffer(fbpair->back);

	int pp_quality = config_get_int(CONFIG_POSTPROCESS);

	ShaderProgram *water_shader = r_shader_get("stage1_water");
	r_uniform_float(r_shader_uniform(water_shader, "time"), 0.5 * global.frames / (float)FPS);
	r_uniform_vec4_rgba(r_shader_uniform(water_shader, "water_color"), &water_color);
	r_uniform_float(r_shader_uniform(water_shader, "wave_offset"), stage_3d_context.cx[1] / 2400.0);

	if(pp_quality > 1) {
		r_shader("blur5");
		r_uniform_vec2("blur_resolution", VIEWPORT_W, VIEWPORT_H);
		r_uniform_vec2("blur_direction", 1, 0);
		draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
		fbpair_swap(fbpair);
		r_framebuffer(fbpair->back);
		r_uniform_vec2("blur_direction", 0, 1);
		draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
		fbpair_swap(fbpair);
		r_framebuffer(fbpair->back);
	}

	if(pp_quality == 1) {
		r_shader("stage1_water");
		r_uniform_float("time", 0.5 * global.frames / (float)FPS);
		draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
		fbpair_swap(fbpair);
	}

	r_mat_proj_pop();

	r_enable(RCAP_DEPTH_TEST);
	r_disable(RCAP_DEPTH_WRITE);

	r_framebuffer(bg_fb);

	if(pp_quality > 1) {
		r_shader("stage1_water");
		r_uniform_float("time", 0.5 * global.frames / (float)FPS);
	} else {
		r_shader_standard();
	}

	r_mat_mv_push();
	r_mat_mv_translate(0, 70 - 140 * hack, 0);
	r_mat_mv_rotate(10 * DEG2RAD, 1, 0, 0);
	r_mat_mv_scale(0.85 / (z + zo), -0.85 / (z + zo), 0.85);
	r_mat_mv_translate(-VIEWPORT_W/2, VIEWPORT_H/4 * hack, 0);
	r_color4(1, 1, 1, 1);
	draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
	r_mat_mv_pop();

	r_mat_mv_pop();
	r_state_pop();
}

static uint stage1_bg_pos(Stage3D *s3d, vec3 p, float maxrange) {
	vec3 q = {0,0,0};
	return single3dpos(s3d, p, INFINITY, q);
}

static void stage1_smoke_draw(vec3 pos) {
	float d = fabsf(pos[1]-stage_3d_context.cx[1]);

	float o = ((d-500)*(d-500))/1.5e7;
	o *= 5 * pow((5000 - d) / 5000, 3);

	if(o < 1.0f/255.0f) {
		return;
	}

	float spin = 0.01 * sin(pos[1]*993.11);

	r_state_push();
	r_shader("sprite_default");
	r_disable(RCAP_DEPTH_TEST);
	r_cull(CULL_BACK);
	r_mat_mv_push();
	r_mat_mv_translate(pos[0]+600*sin(pos[1]), pos[1], pos[2]+600*sin(pos[1]/25.0));
	r_mat_mv_rotate(M_PI/2, -1, 0, 0);
	r_mat_mv_scale(3.5*2, 2*1.5, 1);
	r_mat_mv_rotate(global.frames * spin + M_PI * 2 * sin(pos[1]*321.23), 0, 0, 1);
	r_draw_sprite(&(SpriteParams) {
		.sprite = "stage1/fog",
		.color = RGBA(0.6 * o, 0.7 * o, 0.8 * o, o * 0.5),
	});
	r_mat_mv_pop();
	r_state_pop();
}

static uint stage1_smoke_pos(Stage3D *s3d, vec3 p, float maxrange) {
	vec3 q = {0,0,800};
	vec3 r = {0,200,0};
	return linear3dpos(s3d, p, maxrange/2.0, q, r);
}

static bool stage1_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 0.78, 0.8, 0.85, 1.0);
	r_uniform_float("start", stage1_bg.fog.near);
	r_uniform_float("end", stage1_bg.fog.far);
	r_uniform_float("exponent", 1.0);
	r_uniform_float("sphereness", 0.2);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static inline uint32_t floathash(float f) {
	uint32_t bits = float_to_bits(f);
	return splitmix32(&bits);
}

static void stage1_waterplants_draw(vec3 pos) {
	r_state_push();

	// stateless pseudo-random fuckery
	int tile = floathash(pos[1] * 311.4312) & 1;
	float offs = 600 * sin(2*M_PI*remainder(3214.322211333 * floathash(pos[1]), M_E));
	float d = -55+50*sin(pos[1]/25.0);
	r_mat_mv_push();
	r_mat_mv_translate(pos[0]+offs, pos[1], d);
	r_mat_mv_rotate(2 * M_PI * sin(32.234*pos[1]) + global.frames / 800.0, 0, 0, 1);

	Sprite spr = { 0 };
	spr.w = spr.h = 1;
	spr.tex = get_tex("stage1/waterplants");
	uint tw, th;
	r_texture_get_size(spr.tex, 0, &tw, &th);
	spr.tex_area.w = tw * 0.5;
	spr.tex_area.h = th;
	spr.tex_area.x = spr.tex_area.w * tile;

	float a = 0.8;
	float s = 160 * (1 + 2 * psin(13.12993*pos[1]));
	r_mat_mv_scale(s, s, 1);
	r_depth_func(DEPTH_GREATER);
	r_disable(RCAP_DEPTH_WRITE);
	r_cull(CULL_FRONT);
	r_mat_tex_push();
	r_mat_tex_scale(0.5, 1, 1);
	r_mat_tex_translate(tile, 0, 0);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = &spr,
		.shader = "sprite_default",
		.flip.x = floathash(pos[1] * 231544.213) & 1,
		.flip.y = floathash(pos[1] * 941233.513) & 1,
		.color = RGBA(0.5*a, 0.4*a, 0.5*a, 0.5*a),
	});
	r_mat_tex_pop();
	r_mat_mv_pop();
	r_state_pop();
}

static uint stage1_waterplants_pos(Stage3D *s3d, vec3 p, float maxrange) {
	vec3 q = {0,0,-300};
	vec3 r = {0,150,0};
	return linear3dpos(s3d, p, maxrange/2.0, q, r);
}

static void stage1_snow_draw(vec3 pos) {
	float o = stage1_bg.snow.opacity;
	// float appear_time = 2760;

	if(o < 1.0f/256.0f) {
		return;
	}

	float d = fabsf(pos[1] - stage_3d_context.cx[1]);

	if(fabsf(d) < 500) {
		return;
	}

	float x = ((d+500)*(d+500))/(5000*5000);

	float h0 = floathash(pos[1]+0) / (double)UINT32_MAX;
	float h1 = floathash(pos[1]+1) / (double)UINT32_MAX;
	float h2 = floathash(pos[1]+2) / (double)UINT32_MAX;

	float height = 1 + sawtooth(h0 + global.frames/240.0);

	o *= pow(1 - 1.5 * clamp(height - 1, 0, 1), 5) * x;
	// o *= min(1, (global.frames - appear_time) / 180.0);

	if(o < 1.0f/256.0f) {
		return;
	}

	if(height > 1) {
		height = 1;
	} else {
		height = glm_ease_quad_in(height);
	}

	if(o > 1) {
		o = 1;
	}

	r_state_push();
	r_shader("sprite_default");
	r_disable(RCAP_DEPTH_TEST);
	r_cull(CULL_BACK);
	r_mat_mv_push();
	r_mat_mv_translate(pos[0] + 2200 * sawtooth(h1), pos[1] + 10 * sawtooth(h2), 1200 - 1200 * height);
	r_mat_mv_rotate(M_PI/2, -1, 0, 0);
	r_draw_sprite(&(SpriteParams) {
		.sprite = "part/smoothdot",
		.color = RGBA(o, o, o, 0),
	});
	r_mat_mv_pop();
	r_state_pop();

}

static uint stage1_snow_pos(Stage3D *s3d, vec3 p, float maxrange) {
	vec3 q = {0,0,0};
	vec3 r = {0,15,0};
	return linear3dpos(s3d, p, maxrange, q, r);
}

void stage1_bg_raise_camera(void) {
	stage1_bg.pitch_target = 75;
}

void stage1_bg_enable_snow(void) {
	stage1_bg.snow.opacity_target = 1;
}

void stage1_bg_disable_snow(void) {
	stage1_bg.snow.opacity_target = 0;
}

static void stage1_draw(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 500, 10000);

	Stage3DSegment segs[] = {
		{ stage1_water_draw, stage1_bg_pos },
		{ stage1_waterplants_draw, stage1_waterplants_pos },
		{ stage1_snow_draw, stage1_snow_pos },
		{ stage1_smoke_draw, stage1_smoke_pos },
	};

	stage3d_draw(&stage_3d_context, 10000, ARRAY_SIZE(segs), segs);
}

static void stage1_update(void) {
	stage3d_update(&stage_3d_context);

	stage_3d_context.crot[1] = 2 * sin(global.frames/113.0);
	stage_3d_context.crot[2] = 1 * sin(global.frames/132.0);

	fapproach_asymptotic_p(&stage_3d_context.crot[0], stage1_bg.pitch_target, 0.01, 1e-5);
	fapproach_asymptotic_p(&stage1_bg.fog.near, stage1_bg.fog.near_target, 0.001, 1e-5);
	fapproach_asymptotic_p(&stage1_bg.fog.far, stage1_bg.fog.far_target, 0.001, 1e-5);
	fapproach_p(&stage1_bg.snow.opacity, stage1_bg.snow.opacity_target, 1.0 / 180.0);
}

static void stage1_start(void) {
	stage3d_init(&stage_3d_context, 64);

	FBAttachmentConfig cfg = { 0 };
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.type = TEX_TYPE_RGBA;
	cfg.tex_params.wrap.s = TEX_WRAP_CLAMP;
	cfg.tex_params.wrap.t = TEX_WRAP_CLAMP;

	stage1_bg.water_fbpair.front = stage_add_background_framebuffer("Stage 1 water FB 1", 0.2, 0.5, 1, &cfg);
	stage1_bg.water_fbpair.back = stage_add_background_framebuffer("Stage 1 water FB 2", 0.2, 0.5, 1, &cfg);

	stage1_bg.fog.near_target = 0.5;
	stage1_bg.fog.far_target = 1.5;
	stage1_bg.snow.opacity_target = 0.0;
	stage1_bg.pitch_target = 60;

	stage1_bg.fog.near = 0.2; // stage1_bg.fog.near_target;
	stage1_bg.fog.far = 1.0; // stage1_bg.fog.far_target;
	stage1_bg.snow.opacity = stage1_bg.snow.opacity_target;

	stage_3d_context.crot[0] = stage1_bg.pitch_target;
	stage_3d_context.cx[2] = 700;
	stage_3d_context.cv[1] = 8;
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
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage1/horizon",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"blur5",
		"stage1_water",
		"zbuf_fog",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
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
	stage3d_shutdown(&stage_3d_context);
	memset(&stage1_bg, 0, sizeof(stage1_bg));
}

static void stage1_spellpractice_start(void) {
	stage1_start();

	Boss* cirno = stage1_spawn_cirno(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(cirno, global.stage->spell, true);
	boss_start_attack(cirno, cirno->attacks);
	global.boss = cirno;

	stage_start_bgm("stage1boss");

	stage1_bg_raise_camera();
	stage1_bg_enable_snow();

	stage1_bg.fog.near = stage1_bg.fog.near_target;
	stage1_bg.fog.far = stage1_bg.fog.far_target;
	stage1_bg.snow.opacity = stage1_bg.snow.opacity_target;
	stage_3d_context.crot[0] = stage1_bg.pitch_target;

	INVOKE_TASK_WHEN(&cirno->events.defeated, common_call_func, stage1_bg_disable_snow);
}

static void stage1_spellpractice_events(void) {
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
	.begin = stage1_spellpractice_start,
	.preload = stage1_preload,
	.end = stage1_end,
	.draw = stage1_draw,
	.update = stage1_update,
	.event = stage1_spellpractice_events,
	.shader_rules = stage1_shaders,
};
