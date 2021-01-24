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

Stage3D stage_3d_context;

void stage3d_init(Stage3D *s, uint pos_buffer_size) {
	memset(s, 0, sizeof(*s));
	camera3d_init(&s->cam);
	s->pos_buffer_size = pos_buffer_size;
	s->pos_buffer = calloc(s->pos_buffer_size, sizeof(vec3));
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


void stage3d_apply_transforms(Stage3D *s, mat4 mat) {
	camera3d_apply_transforms(&s->cam, mat);
}

void stage3d_draw_segment(Stage3D *s, SegmentPositionRule pos_rule, SegmentDrawRule draw_rule, float maxrange) {
	uint num = pos_rule(s, s->cam.pos, maxrange);

	for(uint j = 0; j < num; ++j) {
		draw_rule(s->pos_buffer[j]);
	}
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
	free(s->pos_buffer);
}


// this function generates an array of positions on a line so that all points of
// at least distance maxrange are contained.
//
// The equation of the (infinite) line is
//
//     x = support + direction * n
//
// where n is an integer.
//
uint linear3dpos(Stage3D *s3d, vec3 camera, float maxrange, vec3 support, vec3 direction) {
	vec3 support_to_camera;
	glm_vec3_sub(camera, support, support_to_camera);

	const float direction_length2 = glm_vec3_norm2(direction);
	const float projected_cam = glm_vec3_dot(support_to_camera, direction) / direction_length2;
	const int n_closest_to_cam = projected_cam;

	uint size = 0;

	// This is an approximation that does not take into account the distance
	// of the camera to the line. Can be made exact though.
	const int nrange = maxrange/sqrt(direction_length2);

	// draw furthest to closest
	for(int r = 0; r <= nrange; r++) {
		for(int dir = -1; dir <= 1; dir += 2) {
			if(r == 0 && dir > 0) {
				continue;
			}
			int n = n_closest_to_cam + dir*r;
			vec3 extended_direction;
			glm_vec3_scale(direction, n, extended_direction);

			assert(size < s3d->pos_buffer_size);
			glm_vec3_add(support, extended_direction, s3d->pos_buffer[size]);
			++size;

			if(size == s3d->pos_buffer_size) {
				s3d->pos_buffer_size *= 2;
				log_debug("pos_buffer exhausted, reallocating %u -> %u", size, s3d->pos_buffer_size);
				s3d->pos_buffer = realloc(s3d->pos_buffer, sizeof(vec3) * s3d->pos_buffer_size);
			}
		}
	}

	return size;
}

uint single3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p) {
	assume(s3d->pos_buffer_size > 0);

	vec3 d;
	glm_vec3_sub(p, q, d);

	if(glm_vec3_norm2(d) > maxrange * maxrange) {
		return 0;
	} else {
		memcpy(s3d->pos_buffer, p, sizeof(vec3));
		return 1;
	}
}

void skip_background_anim(void (*update_func)(void), int frames, int *timer, int *timer2) {
	int targetframes = *timer + frames;

	while(*timer < targetframes) {
		++*timer;

		if(timer2) {
			++*timer2;
		}

		update_func();
	}
}
