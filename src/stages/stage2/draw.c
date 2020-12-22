/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "draw.h"

#include "renderer/api.h"
#include "stageutils.h"
#include "global.h"

static void stage2_bg_leaves_draw(vec3 pos) {
	r_state_push();

	r_mat_tex_push();
	r_mat_tex_scale(-1, 1, 1);

	r_shader("alpha_depth");
	r_uniform_sampler("tex", "stage2/leaves");

	r_mat_mv_push();
	r_mat_mv_translate(pos[0] - 360, pos[1], pos[2] + 500);
	r_mat_mv_rotate(-160 * DEG2RAD, 0, 1, 0);
	r_mat_mv_translate(-50,0,0);
	r_mat_mv_scale(1000,3000,1);
	r_draw_quad();
	r_mat_mv_pop();

	r_mat_tex_pop();
	r_state_pop();
}

static void stage2_bg_grass_draw(vec3 pos) {
	r_state_push();
	r_disable(RCAP_DEPTH_TEST);
	r_uniform_sampler("tex", "stage2/roadgrass");

	r_mat_mv_push();
	r_mat_mv_translate(pos[0]+250, pos[1], pos[2] + 40);
	r_mat_mv_rotate((pos[2] / 2 - 14) * DEG2RAD, 0, 1, 0);
	r_mat_mv_scale(-500, 2000, 1);
	r_draw_quad();
	r_mat_mv_pop();

	r_state_pop();
}

static void stage2_bg_ground_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate(pos[0] - 50, pos[1], pos[2]);
	r_mat_mv_scale(-1000, 1000, 1);

	r_color4(0.08, 0.0, 0.1, 1.0);
	r_shader_standard_notex();
	r_draw_quad();
	r_shader_standard();
	r_uniform_sampler("tex", "stage2/roadstones");
	r_color4(0.5, 0.5, 0.5, 1);
	r_draw_quad();
	r_color4(1, 1, 1, 1);
	r_mat_mv_translate(0,0,+10);
	r_draw_quad();

	r_mat_mv_pop();

	r_uniform_sampler("tex", "stage2/border");

	r_mat_tex_push();
	r_mat_tex_translate(global.frames / 100.0, sin(global.frames/100.0), 0);
	r_mat_mv_push();
	r_mat_mv_translate(pos[0] + 410, pos[1], pos[2] + 600);
	r_mat_mv_rotate(M_PI/2, 0, 1, 0);
	r_mat_mv_scale(1200,1000,1);
	r_draw_quad();
	r_mat_mv_pop();
	r_mat_tex_pop();

	r_state_pop();
}

static uint stage2_bg_grass_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 2000, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static uint stage2_bg_grass_pos2(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 1234, 40};
	vec3 r = {0, 2000, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static uint stage2_bg_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 1000, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static bool stage2_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 0.05, 0.0, 0.03, 1.0);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage2_bloom(Framebuffer *fb) {
	r_shader("bloom");
	r_uniform_int("samples", 10);
	r_uniform_float("intensity", 0.05);
	r_uniform_float("radius", 0.03);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void stage2_draw(void) {
	Stage3DSegment segs[] = {
		{ stage2_bg_ground_draw, stage2_bg_pos },
		{ stage2_bg_grass_draw, stage2_bg_grass_pos },
		{ stage2_bg_grass_draw, stage2_bg_grass_pos2 },
		{ stage2_bg_leaves_draw, stage2_bg_pos },
	};

	stage3d_draw(&stage_3d_context, 7000, ARRAY_SIZE(segs), segs);
}

void stage2_drawsys_init(void) {
	stage3d_init(&stage_3d_context, 16);
	stage_3d_context.cam.fovy = STAGE3D_DEFAULT_FOVY;
	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT;  // FIXME
	stage_3d_context.cam.near = 500;
	stage_3d_context.cam.far = 5000;
}

void stage2_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
}

ShaderRule stage2_bg_effects[] = {
	stage2_fog,
	stage2_bloom,
	NULL
};
