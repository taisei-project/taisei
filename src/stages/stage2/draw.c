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
#include "util/glm.h"

static Stage2DrawData *stage2_draw_data;

Stage2DrawData *stage2_get_draw_data(void) {
	return NOT_NULL(stage2_draw_data);
}

#define STAGE2_MAX_LIGHTS 4
static void stage2_bg_setup_pbr_lighting(int max_lights) {
	// FIXME: Instead of calling this for each segment, maybe set up the lighting once for the whole scene?

	Camera3D *cam = &stage_3d_context.cam;

	vec3 light_pos[STAGE2_MAX_LIGHTS] = {
		{100,cam->pos[1]-100,100},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	};

	vec3 light_colors[ARRAY_SIZE(light_pos)] = {
		{50000,50000,50000},
		{1, 0, 30},
		{1, 0, 30},
		{1, 0, 30},
	};

	Stage2DrawData *draw_data = stage2_get_draw_data();

	if(global.boss && draw_data->hina_lights > 0) {
		vec3 r;
		camera3d_unprojected_ray(&stage_3d_context.cam, global.boss->pos, r);

		real t = (0.1-cam->pos[2])/r[2];
		glm_vec3_scale(r, t, r);

		vec3 hina_center;
		glm_vec3_add(cam->pos, r, hina_center);

		int hina_light_count = 3;
		for(int i = 0; i < hina_light_count; i++) {
			real angle = 2 * M_PI / hina_light_count * i + global.frames/(real)FPS;
			vec3 offset = {cos(angle), sin(angle)};
			glm_vec3_scale(offset, 4+sin(3*angle), offset);
			glm_vec3_add(hina_center, offset, light_pos[1+i]);

			glm_vec3_scale(light_colors[1+i], draw_data->hina_lights, light_colors[1+i]);
		}
	} else {
		max_lights = 1;
	}

	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(&stage_3d_context.cam, camera_trans);

	r_uniform_mat4("camera_transform", camera_trans);
	r_uniform_vec3_array("light_positions[0]", 0, ARRAY_SIZE(light_pos), light_pos);
	r_uniform_vec3_array("light_colors[0]", 0, ARRAY_SIZE(light_colors), light_colors);
	int light_count = imin(max_lights, ARRAY_SIZE(light_pos));
	r_uniform_int("light_count", light_count);

	r_uniform_vec3("ambient_color",0.5,0.5,0.5);
}

static void stage2_branch_mv_transform(vec3 pos) {
	float f1 = sinf(12123.0f * pos[1] * pos[1]);
	float f2 = cosf(2340.0f * f1 * f1);
	float f3 = cosf(2469.0f * f2 * f2);

	r_mat_mv_translate(pos[0] - f3 * f3, pos[1] + f2, pos[2] + 0.5f * f1);
	r_mat_mv_rotate(-M_PI/2.0f + 0.4f * f1, 0, 0.05f * f3, 1);
}

static void stage2_bg_branch_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	stage2_branch_mv_transform(pos);

	r_shader("pbr");
	stage2_bg_setup_pbr_lighting(1);

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage2/branch_diffuse");
	r_uniform_sampler("roughness_map", "stage2/branch_roughness");
	r_uniform_sampler("normal_map", "stage2/branch_normal");
	r_uniform_sampler("ambient_map", "stage2/branch_ambient");
	r_draw_model("stage2/branch");
	r_mat_mv_pop();

	r_state_pop();
}

static void stage2_bg_leaves_draw(vec3 pos) {
	r_state_push();

// 	r_disable(RCAP_DEPTH_WRITE);

	r_mat_mv_push();
	stage2_branch_mv_transform(pos);

	r_shader("pbr");
	stage2_bg_setup_pbr_lighting(1);

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage2/leaves_diffuse");
	r_uniform_sampler("roughness_map", "stage2/leaves_roughness");
	r_uniform_sampler("normal_map", "stage2/leaves_normal");
	r_uniform_sampler("ambient_map", "stage2/leaves_ambient");
	r_draw_model("stage2/leaves");
	r_mat_mv_pop();

	r_state_pop();
}

static void stage2_bg_grass_draw(vec3 pos) {
	r_state_push();

	float f1 = sinf(123.0f * pos[1] * pos[1]);
	float f2 = cosf(234350.0f * f1 * f1);
	float f3 = cosf(24269.0f * f2 * f2);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0]-f3*f3, pos[1]+f2, pos[2]);
	r_mat_mv_rotate(-M_PI/2 * 1.2, 1, 0, 0);
	r_mat_mv_scale(2, 2, 2);

	ShaderProgram *sprite_shader = res_shader("sprite_pbr");

	r_shader_ptr(sprite_shader);
	stage2_bg_setup_pbr_lighting(STAGE2_MAX_LIGHTS);

	r_disable(RCAP_CULL_FACE);
// 	r_disable(RCAP_DEPTH_WRITE);

	Sprite grass = { 0 };
	grass.tex = res_texture("stage2/grass_diffuse");
	grass.extent.as_cmplx = 1 + I;
	grass.tex_area.extent.as_cmplx = 1 + I;

	SpriteParams sp = { 0 };
	sp.sprite_ptr = &grass;
	sp.color = RGB(1, 1, 1);
	// ambient color and metallicity
	sp.shader_params = &(ShaderCustomParams) {{ 0.4, 0.4, 0.4, 0 }};
	sp.shader_ptr = sprite_shader;
	sp.aux_textures[0] = res_texture("stage2/grass_ambient");
	sp.aux_textures[1] = res_texture("stage2/grass_normal");
	sp.aux_textures[2] = res_texture("stage2/grass_roughness");

	int n = 3;
	for(int i = 0; i < n; i++) {
		r_mat_mv_push();
		r_mat_mv_rotate(M_PI*(i-n/2)/n*0.6,0,1,0);
		r_mat_mv_translate((i-n/2)*0.2*f1,0,0);
		r_draw_sprite(&sp);
		r_mat_mv_pop();
	}

	r_mat_mv_pop();

	r_state_pop();
}

static void stage2_bg_ground_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_shader("pbr");
	stage2_bg_setup_pbr_lighting(STAGE2_MAX_LIGHTS);

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage2/rocks_diffuse");
	r_uniform_sampler("roughness_map", "stage2/rocks_roughness");
	r_uniform_sampler("normal_map", "stage2/rocks_normal");
	r_uniform_sampler("ambient_map", "stage2/rocks_ambient");
	r_draw_model("stage2/rocks");

	r_uniform_sampler("tex", "stage2/ground_diffuse");
	r_uniform_sampler("roughness_map", "stage2/ground_roughness");
	r_uniform_sampler("normal_map", "stage2/ground_normal");
	r_uniform_sampler("ambient_map", "stage2/ground_ambient");
	r_draw_model("stage2/ground");

	r_mat_mv_pop();
	r_state_pop();
}

static void stage2_bg_ground_grass_draw(vec3 pos) {
	r_state_push();

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_shader("pbr");
	stage2_bg_setup_pbr_lighting(STAGE2_MAX_LIGHTS);

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage2/grass_diffuse");
	r_uniform_sampler("roughness_map", "stage2/grass_roughness");
	r_uniform_sampler("normal_map", "stage2/grass_normal");
	r_uniform_sampler("ambient_map", "stage2/grass_ambient");
	r_uniform_vec3("ambient_color", 0.4, 0.4, 0.4);
	r_draw_model("stage2/grass");

	r_mat_mv_pop();
	r_state_pop();
}

#define STAGE2_WATER_SIZE 25

static void stage2_bg_water_draw(vec3 pos) {
	r_state_push();

	r_shader("stage1_water");
	r_uniform_sampler("tex", "stage2/water_floor");
	r_uniform_float("time", 0.2 * global.frames / (float)FPS);

	r_mat_mv_push();
	static const Color water_color = { 0, 0.08, 0.08, 1 };
	r_uniform_vec4_rgba("water_color", &water_color);
	r_uniform_vec2("wave_offset", pos[0]/STAGE2_WATER_SIZE,pos[1]/STAGE2_WATER_SIZE);

	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_scale(-STAGE2_WATER_SIZE,STAGE2_WATER_SIZE,1);
	r_draw_quad();
	r_mat_mv_pop();

	r_state_pop();
}

static uint stage2_bg_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 5, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}
static uint stage2_bg_water_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {7, 8, -1};
	vec3 r = {0, STAGE2_WATER_SIZE, 0};

	return linear3dpos(s3d, pos, maxrange*2, p, r);
}

static uint stage2_bg_water_start_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {7+STAGE2_WATER_SIZE, 6, -1};

	return single3dpos(s3d, pos, maxrange*2, p);
}

static uint stage2_bg_branch_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {-1, 0, 3.5};
	vec3 r = {0, 2, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static uint stage2_bg_grass_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {-2.6, 0, 0.7};
	vec3 r = {0, 0.9, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static bool stage2_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_rgba("fog_color", &stage2_get_draw_data()->fog.color);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 3.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void stage2_draw(void) {
	Stage3DSegment segs[] = {
		{ stage2_bg_branch_draw, stage2_bg_branch_pos },
		{ stage2_bg_ground_draw, stage2_bg_pos},
		{ stage2_bg_water_draw, stage2_bg_water_pos},
		{ stage2_bg_water_draw, stage2_bg_water_start_pos},
		{ stage2_bg_leaves_draw, stage2_bg_branch_pos },
		{ stage2_bg_grass_draw, stage2_bg_grass_pos },
		{ stage2_bg_ground_grass_draw, stage2_bg_pos },
	};

	stage3d_draw(&stage_3d_context, 25, ARRAY_SIZE(segs), segs);
}

void stage2_drawsys_init(void) {
	stage3d_init(&stage_3d_context, 16);
	stage_3d_context.cam.near = 1;
	stage_3d_context.cam.far = 60;
	stage2_draw_data = calloc(1, sizeof(*stage2_draw_data));
}

void stage2_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage2_draw_data);
}

ShaderRule stage2_bg_effects[] = {
	stage2_fog,
	NULL
};
