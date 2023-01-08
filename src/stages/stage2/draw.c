/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "draw.h"

#include "global.h"
#include "renderer/api.h"
#include "resource/model.h"
#include "stageutils.h"
#include "util/glm.h"

static Stage2DrawData *stage2_draw_data;

Stage2DrawData *stage2_get_draw_data(void) {
	return NOT_NULL(stage2_draw_data);
}

#define STAGE2_MAX_LIGHTS 4
#define NUM_HINA_LIGHTS 3

#define TESTLIGHTS 0

#if TESTLIGHTS
#define NUM_TESTLIGHTS 5
#define TESTLIGHT_STRENGTH 100
#define TESTLIGHT_CAMDIST 20
#define TESTLIGHT_CENTERDIST 3
#else
#define NUM_TESTLIGHTS 0
#endif

#define WATER_SIZE 25
#define WATER_ORIGIN { 15, -WATER_SIZE, -0.5 }
#define BRANCH_DIST 2

#if TESTLIGHTS
static void testlights(Camera3D *cam, uint nlights, PointLight3D lights[nlights]) {
	mat4 camera_trans;
	camera3d_apply_inverse_transforms(cam, camera_trans);

	vec3 forward = { 0,  0, -1 };
	vec3 right   = { 1,  0,  0 };
	vec3 up      = { 0,  1,  0 };

	vec3 X = { TESTLIGHT_CENTERDIST, 0, 0 };
	vec3 Y = { 0, TESTLIGHT_CENTERDIST, 0 };

	glm_vec3_rotate_m4(camera_trans, forward, forward);
	glm_vec3_rotate_m4(camera_trans, right, right);
	glm_vec3_rotate_m4(camera_trans, up, up);

	vec3 orig;
	glm_vec3_copy(cam->pos, orig);
	glm_vec3_muladds(forward, TESTLIGHT_CAMDIST, orig);

	cmplxf r = cdir(global.frames * DEG2RAD);
	cmplxf p = cdir(M_TAU / nlights);

	for(int i = 0; i < nlights; ++i) {
		PointLight3D *l = lights + i;
		glm_vec3_scale(HSL(i / (float)nlights, 1, 0.5)->rgb, TESTLIGHT_STRENGTH, l->radiance);
		glm_vec3_copy(orig, l->pos);
		glm_vec3_muladds(X, crealf(r), l->pos);
		glm_vec3_muladds(Y, cimagf(r), l->pos);
		r *= p;
	}
}
#endif

static uint stage2_hina_lights(Camera3D *cam, uint num_lights, PointLight3D lights[num_lights]) {
	Stage2DrawData *draw_data = stage2_get_draw_data();
	float intensity = draw_data->hina_lights;

	if(!global.boss || intensity <= 0) {
		return 0;
	}

	vec3 r;
	camera3d_unprojected_ray(cam, global.boss->pos, r);
	float t = (0.1f - cam->pos[2]) / r[2];
	glm_vec3_scale(r, t, r);

	vec3 hina_center;
	glm_vec3_add(cam->pos, r, hina_center);

	vec3 radiance = { 1, 0, 30 };
	glm_vec3_scale(radiance, intensity, radiance);

	for(int i = 0; i < num_lights; i++) {
		PointLight3D *l = lights + i;
		float angle = M_TAU / num_lights * i + 0.75f * global.frames/(float)FPS;
		vec3 offset = { cosf(angle), sinf(angle) };
		glm_vec3_scale(offset, 4.0f + sinf(3.0f * angle), offset);
		glm_vec3_add(hina_center, offset, l->pos);
		glm_vec3_copy(radiance, l->radiance);
	}

	return num_lights;
}

static void stage2_bg_setup_pbr_lighting(Camera3D *cam, int max_lights) {
	// FIXME: Instead of calling this for each segment, maybe set up the lighting once for the whole scene?
	PointLight3D lights[STAGE2_MAX_LIGHTS + NUM_TESTLIGHTS] = {
		{ { 100, cam->pos[1] - 100, 100 }, { 50000, 50000, 50000 } },
	};

	uint first_hina_light = 1;

	if(max_lights >= first_hina_light + NUM_HINA_LIGHTS) {
		max_lights += stage2_hina_lights(cam, NUM_HINA_LIGHTS, lights + first_hina_light);
		max_lights -= NUM_HINA_LIGHTS;
	}

#if 0
	testlights(cam, NUM_TESTLIGHTS, lights + max_lights);

	float f = 0.1f + 0.9f * (0.5f + 0.5f * sinf(global.frames / 30.0f));

	for(int i = max_lights; i < max_lights + NUM_TESTLIGHTS; ++i) {
		PointLight3D *l = lights + i;
		glm_vec3_scale(l->radiance, f, l->radiance);
	}

	max_lights += NUM_TESTLIGHTS;
#endif

	camera3d_set_point_light_uniforms(cam, imin(max_lights, ARRAY_SIZE(lights)), lights);
}

static void stage2_bg_setup_pbr_env(Camera3D *cam, int max_lights, PBREnvironment *env) {
	stage2_bg_setup_pbr_lighting(cam, max_lights);
	glm_vec3_broadcast(0.5f, env->ambient_color);
	camera3d_apply_inverse_transforms(cam, env->cam_inverse_transform);
	env->environment_map = stage2_draw_data->envmap;
	env->disable_tonemap = true;
}

static void stage2_branch_mv_transform(vec3 pos) {
	uint64_t seed = stage2_draw_data->branch_rng_seed ^ float_to_bits(pos[1]);

#if 0
	RandomState rng;
	rng_init(&rng, seed);
	#define RAND() vrng_f32s(rng_next_p(&rng))
#else
	#define RAND() vrng_f32s((rng_val_t) { splitmix64(&seed) })
#endif

	float r0 = RAND();
	float r1 = RAND();
	float r2 = RAND();
	float r3 = RAND();
	float r4 = RAND();
	float r5 = RAND();
	#undef RAND

	pos[0] += 0.5f * r0 - 0.8f;
	pos[1] += 0.4f * BRANCH_DIST * r1;
	pos[2] += 0.6f * r2 - 0.25f;

	r_mat_mv_translate_v(pos);
	r_mat_mv_rotate(r3 * M_PI/16,    1, 0, 0);
	r_mat_mv_rotate(r4 * M_PI/8    , 0, 0, 1);
	r_mat_mv_rotate(r5 * M_PI/16,    0, 1, 0);
}

static void stage2_bg_branch_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	stage2_branch_mv_transform(pos);

	r_shader("pbr");

	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, 1, &env);

	pbr_draw_model(&stage2_draw_data->models.branch, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_leaves_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	stage2_branch_mv_transform(pos);

	r_shader("pbr_diffuse_alpha_discard");

	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, 1, &env);

	pbr_draw_model(&stage2_draw_data->models.leaves, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_ground_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_blend(BLEND_NONE);
	r_shader("pbr");
	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, STAGE2_MAX_LIGHTS, &env);

	pbr_draw_model(&stage2_draw_data->models.ground, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_ground_rocks_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_blend(BLEND_NONE);
	r_shader("pbr");
	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, STAGE2_MAX_LIGHTS, &env);

	pbr_draw_model(&stage2_draw_data->models.rocks, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_ground_grass_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_CULL_FACE);
	r_shader("pbr_diffuse_alpha_discard");
	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, STAGE2_MAX_LIGHTS, &env);

	pbr_draw_model(&stage2_draw_data->models.grass, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_water_draw(vec3 pos) {
	r_state_push();

	r_shader("pbr_water");
	r_uniform_float("time", 0.2 * global.frames / (float)FPS);
	r_uniform_float("wave_height", 0.06);
	r_uniform_float("wave_scale", 16);
	r_uniform_vec2("wave_offset", pos[0]/WATER_SIZE, pos[1]/WATER_SIZE);
	r_uniform_float("water_depth", 5.0);

	PBREnvironment env = { 0 };
	stage2_bg_setup_pbr_env(&stage_3d_context.cam, STAGE2_MAX_LIGHTS, &env);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_scale(-WATER_SIZE, WATER_SIZE, 1);

	mat4 imv;
	glm_mat4_inv_fast(*r_mat_mv_current_ptr(), imv);
	r_uniform_mat4("inverse_modelview", imv);

	r_mat_tex_push();
	r_mat_tex_translate(pos[0], pos[1], 0);
	pbr_draw_model(&stage2_draw_data->models.water, &env);
	r_mat_tex_pop();
	r_mat_mv_pop();

	r_state_pop();
}

static uint stage2_bg_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 orig = {0, 0, 0};
	vec3 step = {0, 5, 0};

	return stage3d_pos_ray_nearfirst_nsteps(s3d, cam, orig, step, 6, 0);
}

static uint stage2_bg_water_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 orig = WATER_ORIGIN;
	vec3 step = {0, WATER_SIZE, 0};

	return stage3d_pos_ray_nearfirst_nsteps(s3d, cam, orig, step, 10, 10);
}

static uint stage2_bg_water_start_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 o = WATER_ORIGIN;

	float r = maxrange * 20;
	int c = 0;

	for(int x = 1; x <= 2; ++x) {
		for(int y = 0; y <= 3; ++y) {
			vec3 p = { o[0]+x*WATER_SIZE, o[1]+y*WATER_SIZE, o[2] };
			c += stage3d_pos_single(s3d, cam, p, r);
		}
	}

	return c;
}

static uint stage2_bg_branch_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 orig = {-1, 0, 3.5};
	vec3 step = {0, BRANCH_DIST, 0};

	return stage3d_pos_ray_nearfirst_nsteps(s3d, cam, orig, step, 8, 0);
}

static bool stage2_fog(Framebuffer *fb) {
	Stage2DrawData *dd = stage2_get_draw_data();
	r_shader("zbuf_fog_tonemap");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_rgba("fog_color", &dd->fog.color);
	r_uniform_float("start", 0.0);
	r_uniform_float("end", dd->fog.end);
	r_uniform_float("exponent", 24.0);
	r_uniform_float("curvature", 0);

	vec3 exp = { 0.9f, 0.95f, 1.0f };
	glm_vec3_scale(exp, 0.7, exp);
	r_uniform_vec3_vec("exposure", exp);

	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static uint stage2_testlights_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	return stage3d_pos_single(s3d, cam, cam, maxrange);
}

#if TESTLIGHTS
static void stage3d_testlights_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();

	r_shader_standard_notex();
	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_CULL_FACE);

	Camera3D *cam = &stage_3d_context.cam;
	PointLight3D lights[NUM_TESTLIGHTS];
	testlights(cam, ARRAY_SIZE(lights), lights);

	Model *mdl = res_model("cube");
	float scale = 0.05;

	for(int i = 0; i < ARRAY_SIZE(lights); ++i) {
		PointLight3D *l = lights + i;

		r_mat_mv_push();
		r_mat_mv_translate_v(l->pos);
		r_mat_mv_scale(scale, scale, scale);

		Color c = { 0, 0, 0, 1 };
		glm_vec3_divs(l->radiance, TESTLIGHT_STRENGTH, c.rgb);
		color_mul_scalar(&c, 0.5);
		r_color(&c);

		r_draw_model_ptr(mdl, 0, 0);
		r_mat_mv_pop();
	}

	r_mat_mv_pop();
	r_state_pop();
}
#endif

static void stage3d_hinalights_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();

	r_shader_standard_notex();
	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_DEPTH_WRITE);
	r_disable(RCAP_CULL_FACE);

	uint instances = 64;

	const Model *quad = r_model_get_quad();
	Sprite *spr = res_sprite("part/stain");
	r_shader("fireparticles");
	r_uniform_sampler("sprite_tex", spr->tex);
	r_uniform_vec3("color_base", 1, 0, 0);
	r_uniform_vec3("color_nstate", 0, 0, 1);
	r_uniform_vec3("color_nstate2", 0, 0.2, 1);
	r_uniform_vec4("tint", 1, 1, 1, 0);
	r_uniform_vec4("sprite_tex_region",
		spr->tex_area.x,
		spr->tex_area.y,
		spr->tex_area.w,
		spr->tex_area.h
	);
	r_uniform_float("time", global.frames / 120.0f);

	Camera3D *cam = &stage_3d_context.cam;
	PointLight3D lights[NUM_HINA_LIGHTS];
	uint nlights = stage2_hina_lights(cam, ARRAY_SIZE(lights), lights);

	uint32_t seed = 0xabcdef69;

	for(int i = 0; i < nlights; ++i) {
		float s0 = splitmix32(&seed) / (double)UINT32_MAX;
		float s1 = splitmix32(&seed) / (double)UINT32_MAX;
		r_uniform_vec2("seed", s0, s1);

		PointLight3D *l = lights + i;

		r_mat_mv_push();
		r_mat_mv_translate_v(l->pos);
		mat4 *mv = r_mat_mv_current_ptr();
		glm_mul(*mv, stage2_draw_data->hina_fire_emitter_transform, *mv);
		r_draw_model_ptr(quad, instances, 0);
		r_mat_mv_pop();
	}

	r_mat_mv_pop();
	r_state_pop();
}

void stage2_draw(void) {
	Stage3DSegment segs[] = {
		{ stage2_bg_branch_draw, stage2_bg_branch_pos },
		{ stage2_bg_ground_rocks_draw, stage2_bg_pos},
		{ stage2_bg_ground_draw, stage2_bg_pos},
		{ stage2_bg_water_draw, stage2_bg_water_pos},
		{ stage2_bg_water_draw, stage2_bg_water_start_pos},
		{ stage2_bg_leaves_draw, stage2_bg_branch_pos },
		{ stage2_bg_ground_grass_draw, stage2_bg_pos},
		{ stage3d_hinalights_draw, stage2_testlights_pos },
#if TESTLIGHTS
		{ stage3d_testlights_draw, stage2_testlights_pos },
#endif
	};

	stage3d_draw(&stage_3d_context, 20, ARRAY_SIZE(segs), segs);
}

void stage2_drawsys_init(void) {
	stage3d_init(&stage_3d_context, 16);
	stage_3d_context.cam.near = 1;
	stage_3d_context.cam.far = 60;
	stage2_draw_data = aligned_alloc(alignof(*stage2_draw_data), sizeof(*stage2_draw_data));
	memset(stage2_draw_data, 0, sizeof(*stage2_draw_data));

	pbr_load_model(&stage2_draw_data->models.branch, "stage2/branch", "stage2/branch");
	pbr_load_model(&stage2_draw_data->models.grass,  "stage2/grass",  "stage2/ground");
	pbr_load_model(&stage2_draw_data->models.ground, "stage2/ground", "stage2/ground");
	pbr_load_model(&stage2_draw_data->models.leaves, "stage2/leaves", "stage2/leaves");
	pbr_load_model(&stage2_draw_data->models.rocks,  "stage2/rocks",  "stage2/rocks");

	stage2_draw_data->models.water.mdl = (Model*)r_model_get_quad();
	stage2_draw_data->models.water.mat = res_material("stage2/lakefloor");

	mat4 *m = &stage2_draw_data->hina_fire_emitter_transform;
	glm_mat4_identity(*m);
	glm_rotate_x(*m, -M_PI/2, *m);
	float s = 0.6f;
	glm_scale_to(*m, (vec3) { 0.5f * s, s, s }, *m);

	stage2_draw_data->envmap = res_texture("stage2/envmap");
	stage2_draw_data->branch_rng_seed = makeseed();
}

void stage2_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage2_draw_data);
}

ShaderRule stage2_bg_effects[] = {
	stage2_fog,
	NULL
};
