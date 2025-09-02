/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "draw.h"

#include "stageutils.h"
#include "global.h"
#include "util/glm.h"
#include "util/graphics.h"

static Stage3DrawData *stage3_draw_data;

Stage3DrawData *stage3_get_draw_data(void) {
	return NOT_NULL(stage3_draw_data);
}

static uint stage3_bg_pos(Stage3D *s3d, vec3 cam, float maxrange) {
	vec3 orig = { 0, 0, 0, };
	vec3 step = { 0, 20, 10 };

	return stage3d_pos_ray_nearfirst_nsteps(s3d, cam, orig, step, 2, 0);
}

static void stage3_bg_setup_pbr_lighting(Camera3D *cam) {
	PointLight3D lights[2] = {
		{ { 0, 0, 0 }, { 10, 10, 10 } },
	};

	PointLight3D *boss_light = &lights[1];
	uint nlights = 1;

	if(stage3_draw_data->boss_light_alpha > 0) {
		glm_vec3_scale(
			stage3_draw_data->boss_light.radiance,
			stage3_draw_data->boss_light_alpha,
			boss_light->radiance
		);
		glm_vec3_copy(stage3_draw_data->boss_light.pos, boss_light->pos);
		nlights++;
	}

	vec3 r;
	camera3d_unprojected_ray(cam, global.plr.pos, r);
	glm_vec3_scale(r, 5, r);
	glm_vec3_add(cam->pos, r, lights[0].pos);

	camera3d_set_point_light_uniforms(cam, nlights, lights);
}

static void stage3_bg_setup_pbr_env(Camera3D *cam, PBREnvironment *env) {
	Stage3DrawData *dd = stage3_get_draw_data();
	stage3_bg_setup_pbr_lighting(cam);
	glm_vec3_copy(dd->ambient_color, env->ambient_color);
	glm_vec3_copy(dd->environment_color, env->environment_color);
	camera3d_apply_inverse_transforms(cam, env->cam_inverse_transform);
	env->environment_map = stage3_draw_data->envmap;
	env->disable_tonemap = true;
}

static void stage3_bg_ground_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_shader("pbr");

	PBREnvironment env = {};
	stage3_bg_setup_pbr_env(&stage_3d_context.cam, &env);

	pbr_draw_model(&stage3_draw_data->models.trees, &env);
	pbr_draw_model(&stage3_draw_data->models.rocks, &env);
	pbr_draw_model(&stage3_draw_data->models.ground, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static void stage3_bg_leaves_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);
	r_mat_mv_translate(0, 0, -0.0002);

	r_shader("pbr_roughness_alpha_discard");

	PBREnvironment env = {};
	stage3_bg_setup_pbr_env(&stage_3d_context.cam, &env);

	pbr_draw_model(&stage3_draw_data->models.leaves, &env);

	r_mat_mv_pop();
	r_state_pop();
}

void stage3_drawsys_init(void) {
	stage3d_init(&stage_3d_context, 16);
	stage_3d_context.cam.pos[1] = -16;
	stage_3d_context.cam.rot.v[0] = 80;
	stage_3d_context.cam.vel[1] = 0.1;
	stage_3d_context.cam.vel[2] = 0.05;

	stage3_draw_data = ALLOC(typeof(*stage3_draw_data));

	pbr_load_model(&stage3_draw_data->models.ground, "stage3/ground", "stage3/ground");	pbr_load_model(&stage3_draw_data->models.leaves, "stage3/leaves", "stage3/leaves");
	pbr_load_model(&stage3_draw_data->models.rocks,  "stage3/rocks",  "stage3/rocks");
	pbr_load_model(&stage3_draw_data->models.trees,  "stage3/trees",  "stage3/trees");

	stage3_draw_data->envmap = res_texture("stage3/envmap");
}

void stage3_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	mem_free(stage3_draw_data);
}

static bool stage3_fog(Framebuffer *fb) {
	Stage3DrawData *dd = stage3_get_draw_data();
	r_shader("zbuf_fog_tonemap");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_vec("fog_color", dd->fog_color);
	r_uniform_float("start", 0.9);
	r_uniform_float("end", 2);
	r_uniform_float("exponent", 1);
	r_uniform_float("curvature", 0);
	float e = 1;
	r_uniform_vec3("exposure", e, e, e);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage3_glitch(Framebuffer *fb) {
	float strength;

	if(global.boss && global.boss->current && ATTACK_IS_SPELL(global.boss->current->type) && !strcmp(global.boss->name, "Scuttle")) {
		strength = 0.05f * max(0, (global.frames - global.boss->current->starttime) / (float)global.boss->current->timeout);
	} else {
		strength = 0.0;
	}

	if(strength > 0) {
		r_shader("glitch");
		r_uniform_float("strength", strength);
		r_uniform_float("frames", global.frames + 15 * rng_sreal());
	} else {
		return false;
	}

	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void stage3_draw(void) {
	Stage3DSegment segments[] = {
		{ stage3_bg_leaves_draw, stage3_bg_pos },
		{ stage3_bg_ground_draw, stage3_bg_pos },
	};
	r_clear(BUFFER_COLOR, RGB(0.12, 0.11, 0.10), 1);
	stage3d_draw(&stage_3d_context, 120, ARRAY_SIZE(segments), segments);
}

ShaderRule stage3_bg_effects[] = {
	stage3_fog,
	NULL
};

ShaderRule stage3_postprocess[] = {
	stage3_glitch,
	NULL
};
