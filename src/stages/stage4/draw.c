/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "draw.h"

#include "global.h"
#include "stageutils.h"
#include "util/glm.h"
#include "util/graphics.h"

#define LIGHT_OFS    { 2.7f,  4.86f, 0.3f }
#define CORRIDOR_POS { 0.0f, 25.0f, 3.0f }
#define TORCHLIGHT_CLOCK (global.frames / 30.0f)

static Stage4DrawData *stage4_draw_data;

Stage4DrawData *stage4_get_draw_data(void) {
	return NOT_NULL(stage4_draw_data);
}

static bool stage4_fog(Framebuffer *fb) {
	r_state_push();
	r_blend(BLEND_NONE);

	Color c = *RGBA(0.05, 0.0, 0.01, 1.0);

	r_shader("zbuf_fog_tonemap");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_rgba("fog_color", &c);
	r_uniform_float("start", 0.4);
	r_uniform_float("end", 1);
	r_uniform_float("exponent", 20.0);
	r_uniform_float("curvature", 0);
	r_uniform_vec3("exposure", 1, 1, 1);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);

	r_state_pop();

	return true;
}

static bool should_draw_water(void) {
	return stage_3d_context.cam.pos[1] < 0;
}

static bool stage4_water(Framebuffer *fb) {
	if(!should_draw_water()) {
		return false;
	}

	// TODO: SSR-less version for postprocess < 2

	r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
	r_mat_proj_push_perspective(stage_3d_context.cam.fovy, stage_3d_context.cam.aspect, stage_3d_context.cam.near, stage_3d_context.cam.far);
	r_state_push();
	r_mat_mv_push();
	stage3d_apply_transforms(&stage_3d_context, *r_mat_mv_current_ptr());
	r_mat_mv_translate(0, 3, -0.6);
	r_enable(RCAP_DEPTH_TEST);

	r_mat_mv_scale(15, -10, 1);
	r_shader("ssr_water");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_sampler("tex", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_COLOR0));
	r_uniform_float("time", global.frames * 0.002);
	r_uniform_vec2("wave_offset", -global.frames * 0.0005, 0);
	r_uniform_float("wave_height", 0.005);
	r_uniform_sampler("water_noisetex", "fractal_noise");
	r_color4(0.8, 0.9, 1.0, 1);
	r_mat_tex_push();
	r_mat_tex_scale(3, 3, 3);
	r_draw_quad();
	r_mat_tex_pop();

	r_mat_mv_pop();
	r_state_pop();
	r_mat_proj_pop();

	return true;
}

static bool stage4_water_composite(Framebuffer *reflections) {
	if(!should_draw_water()) {
		return false;
	}

	r_state_push();

	// NOTE: Ideally, we should copy depth from the previous framebuffer here,
	// otherwise the fog shader will destroy parts of the water that were composed
	// over the void. We can get away without a proper copy in this case by simply
	// enabling unconditional depth writes. This way, fragments that were not
	// rejected by the alpha threshold test will get a depth value of 0.5 from the
	// screen-space quad. This effectively makes the water fragments completely
	// exempt from the fog effect, which is not 100% correct, but the error is not
	// noticeable with our camera and fog settings.

	// Perhaps a more declarative post-processing system would be nice, so that we
	// could specify which passes consume and/or update depth, and have the depth
	// buffer copied automatically when needed.

	r_enable(RCAP_DEPTH_TEST);
	r_depth_func(DEPTH_ALWAYS);
	r_blend(BLEND_NONE);

	r_shader("alpha_discard");
	r_uniform_float("threshold", 1);
	draw_framebuffer_tex(reflections, VIEWPORT_W, VIEWPORT_H);

	r_state_pop();

	return true;
}

static void torchlight(float t, PointLight3D *light);

static void stage4_bg_setup_pbr_lighting_outdoors(Camera3D *cam) {
	PointLight3D lights[] = {
		{ { 0,  0, 0 },  { 3,  4,  5 } },
		{ { 0, 25, 3 },  { 4*0, 20*0, 22*0} },
	};

	vec3 r;
	camera3d_unprojected_ray(cam, global.plr.pos, r);
	glm_vec3_scale(r, 4, r);
	glm_vec3_add(cam->pos, r, lights[0].pos);

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void stage4_bg_setup_pbr_env_outdoors(Camera3D *cam, PBREnvironment *env) {
	stage4_bg_setup_pbr_lighting_outdoors(cam);
	glm_vec3_broadcast(1.0f, env->ambient_color);
	env->disable_tonemap = true;
}

static void torchlight(float t, PointLight3D *light) {
	uint32_t prng = float_to_bits(light->pos[0]) ^ float_to_bits(light->pos[1]);
	float t_ofs = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	t += 32.124 * t_ofs;

	float o0 = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	float o1 = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	float o2 = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	float o3 = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	float o4 = (float)(splitmix32(&prng) / (double)UINT32_MAX);
	float o5 = (float)(splitmix32(&prng) / (double)UINT32_MAX);

	float r = powf(psinf(2*t+o0), 2)    + powf(pcosf(4.43*t+o1), 4) + powf(psinf(5*t+o2),  21);
	float g = powf(psinf(1.65*t+o3), 2) + powf(pcosf(5.21*t+o4), 3) + powf(psinf(-3*t+o5), 12);

	r = max(r, g);
	r = powf(tanhf(r), 5);
	g = powf(tanhf(g), 5);

	vec3 c;
	vec3 rg = {
		r * stage4_draw_data->corridor.torch_light.r_factor,
		g * stage4_draw_data->corridor.torch_light.g_factor,
		0.0f
	};
	glm_vec3_add(stage4_draw_data->corridor.torch_light.base, rg, c);
	glm_vec3_scale(c, 20 + sinf(t) + cosf(2*t), light->radiance);
}

static void stage4_bg_setup_pbr_lighting_indoors(Camera3D *cam, vec3 pos) {
	vec3 off = LIGHT_OFS;

	PointLight3D lights[] = {
		{ { -off[0], off[1]+pos[1],    pos[2]+off[2] } },
		{ {  off[0], off[1]+pos[1],    pos[2]+off[2] } },
		{ { -off[0], off[1]+pos[1]-10, pos[2]+off[2] } },
		{ {  off[0], off[1]+pos[1]-10, pos[2]+off[2] } },
		{ { -off[0], off[1]+pos[1]+10, pos[2]+off[2] } },
		{ {  off[0], off[1]+pos[1]+10, pos[2]+off[2] } },
	};

	float t = TORCHLIGHT_CLOCK;

	for(int i = 0; i < ARRAY_SIZE(lights); i++) {
		torchlight(t, lights + i);
	}

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void stage4_bg_setup_pbr_env_indoors(Camera3D *cam, vec3 pos, PBREnvironment *env) {
	stage4_bg_setup_pbr_lighting_indoors(cam, pos);
	glm_vec3_copy(stage4_draw_data->ambient_color.rgb, env->ambient_color);
	env->disable_tonemap = true;
}

static uint stage4_lake_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 orig = {0, 0, 0};
	return stage3d_pos_single(s3d, cam, orig, 35);
}

static void stage4_lake_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_shader("pbr");

	PBREnvironment env = {};
	stage4_bg_setup_pbr_env_outdoors(&stage_3d_context.cam, &env);

	pbr_draw_model(&stage4_draw_data->models.ground, &env);
	pbr_draw_model(&stage4_draw_data->models.mansion, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static uint stage4_corridor_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 p = CORRIDOR_POS;
	vec3 r = {0, 10, 0};
	return stage3d_pos_ray_nearfirst(s3d, cam, p, r, maxrange, 0);
}

static uint stage4_flames_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 p = CORRIDOR_POS;
	vec3 r = {0, 10, 0};
	return stage3d_pos_ray_farfirst(s3d, cam, p, r, maxrange, 0);
}

static void stage4_corridor_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_shader("pbr");

	PBREnvironment env = {};
	stage4_bg_setup_pbr_env_indoors(&stage_3d_context.cam, pos, &env);

	pbr_draw_model(&stage4_draw_data->models.corridor, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage4_flames_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();

	Camera3D *cam = &stage_3d_context.cam;
	float dist = pos[1] - cam->pos[1];

	float dmin = 8;
	float dmax = 128;
	float lod = 1.0f - clamp((dist - dmin) / (dmax - dmin), 0.0f, 1.0f);

	uint imin = 16;
	uint imax = 256;
	uint instances = lerpf(imin, imax, lod * lod * lod * lod);

	float al = min(1.0f, 64.0f / instances);

	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_DEPTH_WRITE);

	const Model *quad = r_model_get_quad();
	Sprite *spr = res_sprite("part/stain");
	r_shader("fireparticles");
	r_uniform_sampler("sprite_tex", spr->tex);
	r_uniform_vec3_vec("color_base", stage4_draw_data->corridor.torch_particles.c_base);
	r_uniform_vec3_vec("color_nstate", stage4_draw_data->corridor.torch_particles.c_nstate);
	r_uniform_vec3_vec("color_nstate2", stage4_draw_data->corridor.torch_particles.c_nstate2);
	r_uniform_vec4("tint", al, al, al, al);
	r_uniform_vec4("sprite_tex_region",
		spr->tex_area.x,
		spr->tex_area.y,
		spr->tex_area.w,
		spr->tex_area.h
	);
	r_uniform_float("time", global.frames / 120.0f);

	vec3 ofs = LIGHT_OFS;
	vec3 p;
	mat4 *mv;

	glm_vec3_add(pos, ofs, p);
	r_uniform_vec2_vec("seed", p);
	r_mat_mv_push();
	r_mat_mv_translate_v(p);
	mv = r_mat_mv_current_ptr();
	glm_mul(*mv, stage4_draw_data->fire_emitter_transform, *mv);
	r_draw_model_ptr(quad, instances, 0);
	r_mat_mv_pop();

	ofs[0] *= -1.0f;

	glm_vec3_add(pos, ofs, p);
	r_uniform_vec2_vec("seed", p);
	r_mat_mv_push();
	r_mat_mv_translate_v(p);
	mv = r_mat_mv_current_ptr();
	glm_mul(*mv, stage4_draw_data->fire_emitter_transform, *mv);
	r_draw_model_ptr(quad, instances, 0);
	r_mat_mv_pop();

	r_mat_mv_pop();
	r_state_pop();
}

void stage4_draw(void) {
	Stage3DSegment segs[] = {
		{ stage4_lake_draw, stage4_lake_pos },
		{ stage4_corridor_draw, stage4_corridor_pos },
		{ stage4_flames_draw, stage4_flames_pos },
	};

	stage3d_draw(&stage_3d_context, 240, ARRAY_SIZE(segs), segs);
}

void stage4_drawsys_init(void) {
	stage4_draw_data = ALLOC(typeof(*stage4_draw_data));
	stage3d_init(&stage_3d_context, 16);

	pbr_load_model(&stage4_draw_data->models.corridor, "stage4/corridor", "stage4/corridor");
	pbr_load_model(&stage4_draw_data->models.ground,   "stage4/ground",   "stage4/ground");
	pbr_load_model(&stage4_draw_data->models.mansion,  "stage4/mansion",  "stage4/mansion");

	mat4 *m = &stage4_draw_data->fire_emitter_transform;
	glm_mat4_identity(*m);
	glm_rotate_x(*m, -M_PI/2, *m);
	float s = 0.6f;
	glm_scale_to(*m, (vec3) { 0.5f * s, s, s }, *m);
}

void stage4_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	mem_free(stage4_draw_data);
	stage4_draw_data = NULL;
}

ShaderRule stage4_bg_effects[] = {
	stage4_water,
	stage4_water_composite,
	stage4_fog,
	NULL,
};
