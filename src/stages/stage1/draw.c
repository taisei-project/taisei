/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "draw.h"

#include "stagedraw.h"
#include "stageutils.h"
#include "global.h"
#include "util/glm.h"

static Stage1DrawData *stage1_draw_data;

Stage1DrawData *stage1_get_draw_data(void) {
	return NOT_NULL(stage1_draw_data);
}

void stage1_drawsys_init(void) {
	stage1_draw_data = calloc(1, sizeof(*stage1_draw_data));
	stage3d_init(&stage_3d_context, 64);

	FBAttachmentConfig cfg = { 0 };
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.type = TEX_TYPE_RGBA;
	cfg.tex_params.wrap.s = TEX_WRAP_CLAMP;
	cfg.tex_params.wrap.t = TEX_WRAP_CLAMP;

	stage1_draw_data->water_fbpair.front = stage_add_background_framebuffer(
		"Stage 1 water FB 1", 0.5, 0.5, 1, &cfg);
	stage1_draw_data->water_fbpair.back = stage_add_background_framebuffer(
		"Stage 1 water FB 2", 0.5, 0.5, 1, &cfg);
}

void stage1_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage1_draw_data);
	stage1_draw_data = NULL;
}

static bool reflect_particle_filter(Projectile *part) {
	return !(part->flags & PFLAG_NOREFLECT) && stage_should_draw_particle(part);
}

static bool reflect_draw_predicate(EntityInterface *ent) {
	switch(ent->draw_layer & ~LAYER_LOW_MASK) {
		case LAYER_PLAYER_SLAVE:
		case LAYER_PLAYER_FOCUS:
		case LAYER_PLAYER_SHOT:
		case LAYER_PLAYER_SHOT_HIGH:
			return false;
		default: break;
	}

	switch(ent->type) {
		case ENT_TYPE_ID(Boss):
		case ENT_TYPE_ID(Enemy):
			return true;

		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);

			if(p->type == PROJ_PARTICLE) {
				return reflect_particle_filter(p);
			}

			return false;
		}

		default: return false;
	}

	UNREACHABLE;
}

static void stage1_water_draw(vec3 pos) {
	Stage1DrawData *draw_data = stage1_get_draw_data();

	int pp_quality = config_get_int(CONFIG_POSTPROCESS);

	// don't even ask

	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate(0, pos[1] + 500, 0);
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

	if(pp_quality < 1) {
		r_state_pop();
		r_mat_mv_pop();
		return;
	}

	r_disable(RCAP_DEPTH_TEST);
	r_disable(RCAP_CULL_FACE);

	Framebuffer *bg_fb = r_framebuffer_current();
	FBPair *fbpair = &draw_data->water_fbpair;
	r_framebuffer(fbpair->back);
	r_mat_proj_push();
	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_mat_mv_push_identity();

	float hack = (stage_3d_context.cam.rot.v[0] - 60) / 15.0;

	float z = glm_lerp(0.75, 0.8, hack);
	float zo = glm_lerp(-0.05, -0.3, hack);

	r_mat_mv_translate(VIEWPORT_W * 0.5 * (1 - z), VIEWPORT_H * 0.5 * (1 - z), 0);
	r_mat_mv_scale(z, z, 1);
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 0), 1);
	r_shader("sprite_default");

	ent_draw(reflect_draw_predicate);

	r_mat_mv_pop();

	fbpair_swap(fbpair);
	r_framebuffer(fbpair->back);

	ShaderProgram *water_shader = res_shader("stage1_water");
	r_uniform_float(r_shader_uniform(water_shader, "time"), 0.5 * global.frames / (float)FPS);
	r_uniform_vec4_rgba(r_shader_uniform(water_shader, "water_color"), &water_color);
	r_uniform_vec2(r_shader_uniform(water_shader, "wave_offset"), 0, pos[1] / 2400.0);

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
		r_mat_mv_push_identity();
		r_shader_ptr(water_shader);
		draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
		fbpair_swap(fbpair);
		r_mat_mv_pop();
	}

	r_mat_proj_pop();

	r_enable(RCAP_DEPTH_TEST);
	r_disable(RCAP_DEPTH_WRITE);

	r_framebuffer(bg_fb);

	if(pp_quality > 1) {
		r_shader_ptr(water_shader);
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

static uint stage1_water_pos(Stage3D *s3d, vec3 p, float maxrange) {
	return stage3d_pos_single(s3d, p, p, maxrange);
}

static void stage1_smoke_draw(vec3 pos) {
	float d = fabsf(pos[1] - stage_3d_context.cam.pos[1]);

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

static uint stage1_smoke_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 origin = {0,0,800};
	vec3 step = {0,200,0};
	return stage3d_pos_ray_farfirst(s3d, cam, origin, step, maxrange * 0.5f, 0);
}

static bool stage1_fog(Framebuffer *fb) {
	Stage1DrawData *draw_data = stage1_get_draw_data();

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 0.78, 0.8, 0.85, 1.0);
	r_uniform_float("start", draw_data->fog.near);
	r_uniform_float("end", draw_data->fog.far);
	r_uniform_float("exponent", 1.0);
	r_uniform_float("curvature", 0.2);
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
	spr.tex = res_texture("stage1/waterplants");
	spr.tex_area.w = 0.5f;
	spr.tex_area.h = 1.0f;
	spr.tex_area.x = 0.5f * tile;

	float a = 0.8;
	float s = 160 * (1 + 2 * psin(13.12993*pos[1]));
	r_mat_mv_scale(s, s, 1);
	r_disable(RCAP_DEPTH_TEST);
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

static uint stage1_waterplants_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 origin = {0,0,-300};
	vec3 step = {0,150,0};
	return stage3d_pos_ray_farfirst(s3d, cam, origin, step, maxrange * 0.5f, 0.0f);
}

static void stage1_snow_draw(vec3 pos) {
	Stage1DrawData *draw_data = stage1_get_draw_data();
	float o = draw_data->snow.opacity;

	if(o < 1.0f/256.0f) {
		return;
	}

	float d = fabsf(pos[1] - stage_3d_context.cam.pos[1]);

	if(fabsf(d) < 500) {
		return;
	}

	float x = ((d+500)*(d+500))/(5000*5000);

	float h0 = floathash(pos[1]+0) / (double)UINT32_MAX;
	float h1 = floathash(pos[1]+1) / (double)UINT32_MAX;
	float h2 = floathash(pos[1]+2) / (double)UINT32_MAX;

	float height = 1 + sawtooth(h0 + global.frames/280.0);

	o *= pow(1 - 1.5 * clamp(height - 1, 0, 1), 5) * x;

	if(o < 1.0f/256.0f) {
		return;
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

static uint stage1_snow_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 origin = {0,0,0};
	vec3 step = {0,15,0};
	return stage3d_pos_ray_farfirst(s3d, cam, origin, step, maxrange, 0);
}

void stage1_draw(void) {
	Stage3DSegment segs[] = {
		{ stage1_water_draw, stage1_water_pos },
		{ stage1_waterplants_draw, stage1_waterplants_pos },
		{ stage1_snow_draw, stage1_snow_pos },
		{ stage1_smoke_draw, stage1_smoke_pos },
	};

	stage3d_draw(&stage_3d_context, 10000, ARRAY_SIZE(segs), segs);
}

ShaderRule stage1_bg_effects[] = {
	stage1_fog,
	NULL
};
