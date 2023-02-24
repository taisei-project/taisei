/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stageutils.h"
#include "global.h"
#include "util/glm.h"
#include "video.h"
#include "resource/model.h"
#include "resource/material.h"

Stage3D stage_3d_context;

void stage3d_init(Stage3D *s, uint pos_buffer_size) {
	memset(s, 0, sizeof(*s));
	camera3d_init(&s->cam);
	dynarray_ensure_capacity(&s->positions, pos_buffer_size);
}

void camera3d_init(Camera3D *cam) {
	memset(cam, 0, sizeof(*cam));
	cam->fovy = STAGE3D_DEFAULT_FOVY;
	cam->aspect = VIEWPORT_W/(real)VIEWPORT_H;
	cam->near = STAGE3D_DEFAULT_NEAR;
	cam->far = STAGE3D_DEFAULT_FAR;
}

void camera3d_update(Camera3D *cam) {
	glm_vec3_add(cam->pos, cam->vel, cam->pos);
}

void stage3d_update(Stage3D *s) {
	camera3d_update(&s->cam);
}

void camera3d_apply_transforms(Camera3D *cam, mat4 mat) {
	glm_rotate_x(mat, glm_rad(-cam->rot.pitch), mat);
	glm_rotate_y(mat, glm_rad(-cam->rot.yaw), mat);
	glm_rotate_z(mat, glm_rad(-cam->rot.roll), mat);

	vec3 trans;
	glm_vec3_negate_to(cam->pos, trans);
	glm_translate(mat, trans);
}

void stage3d_apply_transforms(Stage3D *s, mat4 mat) {
	camera3d_apply_transforms(&s->cam, mat);
}

void camera3d_apply_inverse_transforms(Camera3D *cam, mat4 mat) {
	// TODO optimize this
	mat4 temp;
	glm_mat4_identity(temp);
	camera3d_apply_transforms(cam, temp);
	glm_mat4_inv_fast(temp, mat);
}

void stage3d_apply_inverse_transforms(Stage3D *s, mat4 mat) {
	camera3d_apply_inverse_transforms(&s->cam, mat);
}


// The author that brought you linear3dpos and that one function
// that calculates the closest point to a line segment proudly presents:
//
// camera3d_unprojected_ray!
//
// The purpose of this function is to calculate the ray
//
//     x = camera_position + dest * lambda,   lambda > 0
//
// of points in world space that are shown directly behind a position in the
// 2D viewport. This function returns the normalized vector dest from which
// all points x can be constructed by the user.
// The “unprojected ray” is useful for placing 3D objects that somehow correspond
// to a thing happening in the viewport.
//
// Actually, glm implements most of what is needed for this. Nice!
//
void camera3d_unprojected_ray(Camera3D *cam, cmplx pos, vec3 dest) {
	vec4 viewport = {0, VIEWPORT_H, VIEWPORT_W, -VIEWPORT_H};
	vec3 p = {creal(pos), cimag(pos),1};

	mat4 mpersp;
	glm_perspective(cam->fovy, cam->aspect, cam->near, cam->far, mpersp);
	camera3d_apply_transforms(cam, mpersp);
	glm_translate(mpersp, cam->pos);

	glm_unproject(p, mpersp, viewport, dest);
	glm_vec3_normalize(dest);
}


void camera3d_fill_point_light_uniform_vectors(
	Camera3D *cam,
	uint num_lights,
	PointLight3D lights[num_lights],
	vec3 out_lpos[num_lights],
	vec3 out_lrad[num_lights]
) {
	assume(num_lights <= STAGE3D_MAX_LIGHTS);

	// TODO: persist matrix in Camera3D struct
	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(cam, camera_trans);

	for(int i = 0; i < num_lights; ++i) {
		glm_mat4_mulv3(camera_trans, lights[i].pos, 1, out_lpos[i]);
		glm_vec3_copy(lights[i].radiance, out_lrad[i]);
	}
}

void camera3d_set_point_light_uniforms(
	Camera3D *cam,
	uint num_lights,
	PointLight3D lights[num_lights]
) {
	if(UNLIKELY(num_lights == 0)) {
		return;
	}

	vec3 lpos[num_lights];
	vec3 lrad[num_lights];
	camera3d_fill_point_light_uniform_vectors(cam, num_lights, lights, lpos, lrad);

	r_uniform_vec3_array("light_positions[0]", 0, num_lights, lpos);
	r_uniform_vec3_array("light_colors[0]", 0, num_lights, lrad);
	r_uniform_int("light_count", num_lights);
}

void pbr_set_material_uniforms(const PBRMaterial *m, const PBREnvironment *env)  {
	int flags = 0;

	if(m->diffuse_map) {
		r_uniform_sampler("diffuse_map", m->diffuse_map);
		flags |= PBR_FEATURE_DIFFUSE_MAP;
	}

	if(m->normal_map) {
		r_uniform_sampler("normal_map", m->normal_map);
		flags |= PBR_FEATURE_NORMAL_MAP;
	}

	if(m->roughness_map) {
		r_uniform_sampler("roughness_map", m->roughness_map);
		flags |= PBR_FEATURE_ROUGHNESS_MAP;
	}

	if(m->ambient_map) {
		r_uniform_sampler("ambient_map", m->ambient_map);
		flags |= PBR_FEATURE_AMBIENT_MAP;
	}

	if(m->depth_map && m->depth_scale) {
		r_uniform_sampler("depth_map", m->depth_map);
		flags |= PBR_FEATURE_DEPTH_MAP;
	}

	if(env->environment_map) {
		r_uniform_sampler("ibl_brdf_lut", "ibl_brdf_lut");
		r_uniform_sampler("environment_map", env->environment_map);
		r_uniform_mat4("inv_camera_transform", (vec4*)env->cam_inverse_transform);
		flags |= PBR_FEATURE_ENVIRONMENT_MAP;
	}

	if(m->ao_map) {
		r_uniform_sampler("ao_map", m->ao_map);
		flags |= PBR_FEATURE_AO_MAP;
	}

	if(!env->disable_tonemap) {
		flags |= PBR_FEATURE_NEED_TONEMAP;
	}

	vec4 diffuseRGB_metallicA;
	glm_vec3_copy((float*)m->diffuse_color, diffuseRGB_metallicA);
	diffuseRGB_metallicA[3] = m->metallic_value;
	r_uniform_vec4_vec("diffuseRGB_metallicA", diffuseRGB_metallicA);

	vec4 ambientRGB_roughnessA;
	glm_vec3_mul((float*)env->ambient_color, (float*)m->ambient_color, ambientRGB_roughnessA);
	ambientRGB_roughnessA[3] = m->roughness_value;
	r_uniform_vec4_vec("ambientRGB_roughnessA", ambientRGB_roughnessA);

	vec4 environmentRGB_depthScale;
	glm_vec3_copy((float*)env->environment_color, (float*)environmentRGB_depthScale);
	environmentRGB_depthScale[3] = m->depth_scale;
	r_uniform_vec4_vec("environmentRGB_depthScale", environmentRGB_depthScale);

	r_uniform_int("features_mask", flags);
}

void pbr_draw_model(const PBRModel *pmdl, const PBREnvironment *env) {
	pbr_set_material_uniforms(NOT_NULL(pmdl->mat), env);
	r_draw_model_ptr(NOT_NULL(pmdl->mdl), 0, 0);
}

void pbr_load_model(PBRModel *pmdl, const char *model_name, const char *mat_name) {
	pmdl->mdl = res_model(model_name);
	pmdl->mat = res_material(mat_name);
}

void stage3d_draw_segment(Stage3D *s, SegmentPositionRule pos_rule, SegmentDrawRule draw_rule, float maxrange) {
	s->positions.num_elements = 0;

	// TODO maybe get rid of the return value
	uint num = pos_rule(s, s->cam.pos, maxrange);

	if(num < s->positions.num_elements) {
		s->positions.num_elements = num;
	}

	dynarray_foreach_elem(&s->positions, vec3 *p, {
		draw_rule(*p);
	});
}

void stage3d_draw(Stage3D *s, float maxrange, uint nsegments, const Stage3DSegment segments[nsegments]) {
	r_mat_mv_push();
	stage3d_apply_transforms(s, *r_mat_mv_current_ptr());
	r_mat_proj_push_perspective(s->cam.fovy, s->cam.aspect, s->cam.near, s->cam.far);

	for(uint i = 0; i < nsegments; ++i) {
		const Stage3DSegment *seg = segments + i;
		stage3d_draw_segment(s, seg->pos, seg->draw, maxrange);
	}

	r_mat_mv_pop();
	r_mat_proj_pop();
}

void stage3d_shutdown(Stage3D *s) {
	dynarray_free_data(&s->positions);
}

struct pos_ray_data {
	int n_closest_to_cam;
	int forward_nrange;
	int back_nrange;
	int max_nrange;
};

static struct pos_ray_data pos_ray_init(
	vec3 camera, vec3 origin, vec3 step, float forward_range, float back_range
) {
	assert(forward_range >= 0);
	assert(back_range >= 0);

	vec3 orig_to_cam;
	glm_vec3_sub(camera, origin, orig_to_cam);

	const float step_len2 = glm_vec3_norm2(step);
	const float step_len = sqrtf(step_len2);
	const float projected_cam = glm_vec3_dot(orig_to_cam, step) / step_len2;

	struct pos_ray_data rd = {
		.n_closest_to_cam = roundf(projected_cam),

		// This is an approximation that does not take into account the distance
		// of the camera to the ray. Can be made exact though.
		.forward_nrange = forward_range / step_len,
		.back_nrange = back_range / step_len,
	};
	rd.max_nrange = imax(rd.forward_nrange, rd.back_nrange);

	return rd;
}

static struct pos_ray_data pos_ray_init_nsteps(
	vec3 camera, vec3 origin, vec3 step, int forward_steps, int back_steps
) {
	assert(forward_steps >= 0);
	assert(back_steps >= 0);

	vec3 orig_to_cam;
	glm_vec3_sub(camera, origin, orig_to_cam);

	const float step_len2 = glm_vec3_norm2(step);
	const float projected_cam = glm_vec3_dot(orig_to_cam, step) / step_len2;

	struct pos_ray_data rd = {
		.n_closest_to_cam = roundf(projected_cam),
		.forward_nrange = forward_steps,
		.back_nrange = back_steps,
	};
	rd.max_nrange = imax(rd.forward_nrange, rd.back_nrange);

	return rd;
}

static void pos_ray_step(
	Stage3D *s3d, struct pos_ray_data *rd, int nofs, vec3 step, vec3 origin
) {
	if(nofs > rd->forward_nrange || nofs < -rd->back_nrange) {
		return;
	}

	int n = rd->n_closest_to_cam + nofs;

	if(n < 0) {
		return;
	}

	vec3 ofs;
	glm_vec3_scale(step, n, ofs);
	glm_vec3_add(origin, ofs, *dynarray_append(&s3d->positions));
}

static uint pos_ray_steps_nearfirst(Stage3D *s3d, struct pos_ray_data *d, vec3 origin, vec3 step) {
	const uint prev_size = s3d->positions.num_elements;

	pos_ray_step(s3d, d, 0, step, origin);

	for(int i = 1; i <= d->max_nrange; ++i) {
		for(int dir = -1; dir <= 1; dir += 2) {
			pos_ray_step(s3d, d, dir * i, step, origin);
		}
	}

	return s3d->positions.num_elements - prev_size;
}

static uint pos_ray_steps_farfirst(Stage3D *s3d, struct pos_ray_data *d, vec3 origin, vec3 step) {
	const uint prev_size = s3d->positions.num_elements;

	for(int i = d->max_nrange; i > 0; --i) {
		for(int dir = 1; dir >= -1; dir -= 2) {
			pos_ray_step(s3d, d, dir * i, step, origin);
		}
	}

	pos_ray_step(s3d, d, 0, step, origin);

	return s3d->positions.num_elements - prev_size;
}

uint stage3d_pos_ray_nearfirst(
	Stage3D *s3d, vec3 camera, vec3 origin, vec3 step, float forward_range, float back_range
) {
	struct pos_ray_data d = pos_ray_init(camera, origin, step, forward_range, back_range);
	return pos_ray_steps_nearfirst(s3d, &d, origin, step);
}

uint stage3d_pos_ray_nearfirst_nsteps(
	Stage3D *s3d, vec3 camera, vec3 origin, vec3 step, int forward_steps, int back_steps
) {
	struct pos_ray_data d = pos_ray_init_nsteps(camera, origin, step, forward_steps, back_steps);
	return pos_ray_steps_nearfirst(s3d, &d, origin, step);
}

uint stage3d_pos_ray_farfirst(
	Stage3D *s3d, vec3 camera, vec3 origin, vec3 step, float forward_range, float back_range
) {
	struct pos_ray_data d = pos_ray_init(camera, origin, step, forward_range, back_range);
	return pos_ray_steps_farfirst(s3d, &d, origin, step);
}

uint stage3d_pos_ray_farfirst_nsteps(
	Stage3D *s3d, vec3 camera, vec3 origin, vec3 step, int forward_steps, int back_steps
) {
	struct pos_ray_data d = pos_ray_init_nsteps(camera, origin, step, forward_steps, back_steps);
	return pos_ray_steps_farfirst(s3d, &d, origin, step);
}

uint stage3d_pos_single(Stage3D *s3d, vec3 camera, vec3 origin, float maxrange) {
	vec3 d;
	glm_vec3_sub(origin, camera, d);

	if(glm_vec3_norm2(d) > maxrange * maxrange) {
		return 0;
	} else {
		glm_vec3_copy(origin, *dynarray_append(&s3d->positions));
		return 1;
	}
}
