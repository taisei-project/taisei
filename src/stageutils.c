/*
 * This software is licensed under the terms of the MIT-License
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

void init_stage3d(Stage3D *s, uint pos_buffer_size) {
	memset(s, 0, sizeof(Stage3D));
	s->projangle = 45;
	s->pos_buffer_size = pos_buffer_size;
	s->pos_buffer = calloc(s->pos_buffer_size, sizeof(vec3));
}

void add_model(Stage3D *s, SegmentDrawRule draw, SegmentPositionRule pos) {
	s->models = realloc(s->models, (++s->msize)*sizeof(StageSegment));

	s->models[s->msize - 1].draw = draw;
	s->models[s->msize - 1].pos = pos;
}

void set_perspective_viewport(Stage3D *s, float n, float f, int vx, int vy, int vw, int vh) {
	r_mat_mode(MM_PROJECTION);

	r_mat_identity();
	float facw = SCREEN_W/(float)VIEWPORT_W;
	float fach = SCREEN_H/(float)VIEWPORT_H;
	r_mat_perspective(glm_rad(s->projangle), 1, n, f);
	r_mat_scale(facw,fach,1);
	r_mat_translate(vx+vw/2.0, vy+vh/2.0, 0);

	r_mat_mode(MM_MODELVIEW);
}

void set_perspective(Stage3D *s, float n, float f) {
	set_perspective_viewport(s, n, f, VIEWPORT_X, VIEWPORT_Y, VIEWPORT_W, VIEWPORT_H);
}

void update_stage3d(Stage3D *s) {
	for(int i = 0; i < 3; i++){
		s->cx[i] += s->cv[i];
	}
}

void draw_stage3d(Stage3D *s, float maxrange) {
	r_mat_push();

	if(s->crot[0])
		r_mat_rotate_deg(-s->crot[0], 1, 0, 0);
	if(s->crot[1])
		r_mat_rotate_deg(-s->crot[1], 0, 1, 0);
	if(s->crot[2])
		r_mat_rotate_deg(-s->crot[2], 0, 0, 1);

	if(s->cx[0] || s->cx[1] || s->cx[2])
		r_mat_translate(-s->cx[0],-s->cx[1],-s->cx[2]);

	for(uint i = 0; i < s->msize; i++) {
		uint num = s->models[i].pos(s, s->cx, maxrange);

		for(uint j = 0; j < num; ++j) {
			s->models[i].draw(s->pos_buffer[j]);
		}
	}

	r_mat_pop();
}

void free_stage3d(Stage3D *s) {
	free(s->models);
	free(s->pos_buffer);
}

uint linear3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p, vec3 r) {
	float n = 0, z = 0;

	for(uint i = 0; i < 3; i++) {
		n += q[i]*r[i] - p[i]*r[i];
		z += r[i]*r[i];
	}

	float t = n/z;
	int mod = 1;
	int num = t;
	uint size = 0;

	while(size < s3d->pos_buffer_size) {
		vec3 dif;

		for(uint i = 0; i < 3; i++) {
			dif[i] = q[i] - p[i] - r[i]*num;
		}

		if(glm_vec3_norm(dif) < maxrange) {
			for(uint i = 0; i < 3; i++) {
				s3d->pos_buffer[size][i] = p[i] + r[i]*num;
			}

			++size;
		} else if(mod == 1) {
			mod = -1;
			num = t;
		} else {
			break;
		}

		num += mod;
	}

	return size;
}

uint single3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p) {
	assume(s3d->pos_buffer_size > 0);
	vec3 d;

	for(uint i = 0; i < 3; i++) {
		d[i] = p[i] - q[i];
	}

	if(glm_vec3_norm(d) > maxrange) {
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
