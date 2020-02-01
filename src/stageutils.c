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
	s->pos_buffer_size = pos_buffer_size;
	s->pos_buffer = calloc(s->pos_buffer_size, sizeof(vec3));
}

void stage3d_set_perspective_viewport(Stage3D *s, float n, float f, int vx, int vy, int vw, int vh) {
	float facw = SCREEN_W/(float)VIEWPORT_W;
	float fach = SCREEN_H/(float)VIEWPORT_H;
	r_mat_proj_perspective(M_PI/4, 1, n, f);
	r_mat_proj_scale(facw, fach, 1);
	r_mat_proj_translate(vx + vw / 2.0, vy + vh / 2.0, 0);
}

void stage3d_set_perspective(Stage3D *s, float n, float f) {
	stage3d_set_perspective_viewport(s, n, f, VIEWPORT_X, VIEWPORT_Y, VIEWPORT_W, VIEWPORT_H);
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

void stage3d_draw_segment(Stage3D *s, SegmentPositionRule pos_rule, SegmentDrawRule draw_rule, float maxrange) {
	uint num = pos_rule(s, s->cam.pos, maxrange);

	for(uint j = 0; j < num; ++j) {
		draw_rule(s->pos_buffer[j]);
	}
}

void stage3d_draw(Stage3D *s, float maxrange, uint nsegments, const Stage3DSegment segments[nsegments]) {
	r_mat_mv_push();
	stage3d_apply_transforms(s, *r_mat_mv_current_ptr());

	for(uint i = 0; i < nsegments; ++i) {
		const Stage3DSegment *seg = segments + i;
		stage3d_draw_segment(s, seg->pos, seg->draw, maxrange);
	}

	r_mat_mv_pop();
}

void stage3d_shutdown(Stage3D *s) {
	free(s->pos_buffer);
}

uint linear3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p, vec3 r) {
	const float maxrange_squared = maxrange * maxrange;

	vec3 q_minus_p, temp;
	glm_vec3_sub(q, p, q_minus_p);
	glm_vec3_mul(q_minus_p, r, temp);
	const float n = glm_vec3_hadd(temp);
	const float z = glm_vec3_norm2(r);
	const float t = n/z;

	int mod = 1;
	int num = t;
	uint size = 0;

	while(size < s3d->pos_buffer_size) {
		vec3 dif;
		vec3 r_x_num;
		glm_vec3_scale(r, num, r_x_num);
		glm_vec3_sub(q_minus_p, r_x_num, dif);

		if(glm_vec3_norm2(dif) < maxrange_squared) {
			glm_vec3_add(p, r_x_num, s3d->pos_buffer[size]);
			++size;

			if(size == s3d->pos_buffer_size) {
				s3d->pos_buffer_size *= 2;
				log_debug("pos_buffer exhausted, reallocating %u -> %u", size, s3d->pos_buffer_size);
				s3d->pos_buffer = realloc(s3d->pos_buffer, sizeof(vec3) * s3d->pos_buffer_size);
			}
		} else if(mod == 1) {
			mod = -1;
			num = t;
		} else {
			break;
		}

		num += mod;
	}

	assert(size < s3d->pos_buffer_size);

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
