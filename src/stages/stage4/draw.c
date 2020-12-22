/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "draw.h"
#include "stageutils.h"
#include "global.h"
#include "util/glm.h"

static Stage4DrawData *stage4_draw_data;

Stage4DrawData *stage4_get_draw_data(void) {
	return NOT_NULL(stage4_draw_data);
}

static bool stage4_fog(Framebuffer *fb) {
	float f = 0;

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 9.0*f+0.1, 0.0, 0.1-f, 1.0);
	r_uniform_float("start", 0.4);
	r_uniform_float("end", 1);
	r_uniform_float("exponent", 50.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	return true;
}

static uint stage4_lake_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	return single3dpos(s3d, pos, maxrange, p);
}

static void stage4_lake_draw(vec3 pos) {
	vec3 light_pos[] = {
		{0, stage_3d_context.cam.pos[1]+3, stage_3d_context.cam.pos[2]-0.8},
		{0, 25, 3}
	};

	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(&stage_3d_context.cam, camera_trans);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_shader("pbr");

	vec3 light_colors[] = {
		{1, 22, 22},
		{4, 20, 22},
	};

	vec3 cam_light_positions[2];
	glm_mat4_mulv3(camera_trans, light_pos[0], 1, cam_light_positions[0]);
	glm_mat4_mulv3(camera_trans, light_pos[1], 1, cam_light_positions[1]);


	r_uniform_vec3_array("light_positions[0]", 0, ARRAY_SIZE(cam_light_positions), cam_light_positions);
	r_uniform_vec3_array("light_colors[0]", 0, ARRAY_SIZE(light_colors), light_colors);
	r_uniform_int("light_count", 2);

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/ground_diffuse");
	r_uniform_sampler("roughness_map", "stage4/ground_roughness");
	r_uniform_sampler("normal_map", "stage4/ground_normal");
	r_uniform_sampler("ambient_map", "stage4/ground_ambient");
	r_uniform_vec3("ambient_color", 1, 1, 1);


	r_draw_model("stage4/ground");

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/mansion_diffuse");
	r_uniform_sampler("roughness_map", "stage4/mansion_roughness");
	r_uniform_sampler("normal_map", "stage4/mansion_normal");
	r_uniform_sampler("ambient_map", "stage4/mansion_ambient");

	r_draw_model("stage4/mansion");
	r_mat_mv_pop();
	r_shader_standard();
}

static uint stage4_corridor_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 25, 3};
	vec3 r = {0, 10, 0};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][1] < p[1]) {
			s3d->pos_buffer[i][1] = -9000;
		}
	}

	return num;
}

static void stage4_corridor_draw(vec3 pos) {
	real xoff = 3;
	real zoff = 1.3;
	vec3 light_pos[] = {
		{-xoff, pos[1], pos[2]+zoff},
		{xoff, pos[1], pos[2]+zoff},
		{-xoff, pos[1]-10, pos[2]+zoff},
		{xoff, pos[1]-10, pos[2]+zoff},
		{-xoff, pos[1]+10, pos[2]+zoff},
		{xoff, pos[1]+10, pos[2]+zoff},
	};

	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(&stage_3d_context.cam, camera_trans);


	vec3 light_colors[] = {
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
	};

	vec3 cam_light_positions[ARRAY_SIZE(light_pos)];
	for(int i = 0; i < ARRAY_SIZE(light_pos); i++) {
		glm_mat4_mulv3(camera_trans, light_pos[i], 1, cam_light_positions[i]);

		real t = global.frames*0.02;
		real mod1 = cos(13095434*light_pos[i][1]);
		real mod2 = sin(1242435*light_pos[i][0]*light_pos[i][1]);

		double f = (sin((1+mod1)*t) + sin((2.35+mod2)*t+mod1) + sin((3.1257+mod1*mod2)*t+mod2))/3;
		glm_vec3_scale(light_colors[i],0.6+0.4*f, light_colors[i]);
	}


	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	//r_mat_mv_rotate(pos[1]/2000, 0, 1, 0);
	r_shader("pbr");

	r_uniform_vec3_array("light_positions[0]", 0, ARRAY_SIZE(cam_light_positions), cam_light_positions);
	r_uniform_vec3_array("light_colors[0]", 0, ARRAY_SIZE(light_colors), light_colors);
	r_uniform_int("light_count", ARRAY_SIZE(light_pos));


	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/corridor_diffuse");
	r_uniform_sampler("roughness_map", "stage4/corridor_roughness");
	r_uniform_sampler("normal_map", "stage4/corridor_normal");
	r_uniform_sampler("ambient_map", "stage4/corridor_ambient");
	r_uniform_vec3_rgb("ambient_color", &stage4_draw_data->ambient_color);

	r_draw_model("stage4/corridor");
	r_mat_mv_pop();
}

void stage4_draw(void) {
	Stage3DSegment segs[] = {
		{ stage4_lake_draw, stage4_lake_pos },
		{ stage4_corridor_draw, stage4_corridor_pos },
	};

	stage3d_draw(&stage_3d_context, 400, ARRAY_SIZE(segs), segs);
}

void stage4_drawsys_init(void) {
	stage4_draw_data = calloc(1, sizeof(*stage4_draw_data));
	stage3d_init(&stage_3d_context, 16);
}

void stage4_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage4_draw_data);
	stage4_draw_data = NULL;
}

ShaderRule stage4_bg_effects[] = {
	stage4_fog,
	NULL,
};
