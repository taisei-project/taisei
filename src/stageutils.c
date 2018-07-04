/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stageutils.h"
#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "util/glm.h"
#include "video.h"

Stage3D stage_3d_context;

void init_stage3d(Stage3D *s) {
	memset(s, 0, sizeof(Stage3D));
	s->projangle = 45;
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

	for(int i = 0; i < s->msize; i++) {
		vec3 **list;
		list = s->models[i].pos(s->cx, maxrange);

		for(int j = 0; list && list[j] != NULL; j++) {
			s->models[i].draw(*list[j]);
			free(list[j]);
		}

		free(list);
	}

	r_mat_pop();
}

void free_stage3d(Stage3D *s) {
	free(s->models);
}

vec3 **linear3dpos(vec3 q, float maxrange, vec3 p, vec3 r) {
	int i;
	float n = 0, z = 0;
	for(i = 0; i < 3; i++) {
		n += q[i]*r[i] - p[i]*r[i];
		z += r[i]*r[i];
	}

	float t = n/z;

	vec3 **list = NULL;
	int size = 0;
	int mod = 1;

	int num = t;
	while(1) {
		vec3 dif;

		for(i = 0; i < 3; i++)
			dif[i] = q[i] - p[i] - r[i]*num;

		if(glm_vec_norm(dif) < maxrange) {
			list = realloc(list, (++size)*sizeof(vec3*));
			list[size-1] = malloc(sizeof(vec3));
			for(i = 0; i < 3; i++)
				(*list[size-1])[i] = p[i] + r[i]*num;
		} else if(mod == 1) {
			mod = -1;
			num = t;
		} else {
			break;
		}

		num += mod;
	}

	list = realloc(list, (++size)*sizeof(vec3*));
	list[size-1] = NULL;

	return list;
}

vec3 **single3dpos(vec3 q, float maxrange, vec3 p) {
	vec3 d;

	int i;

	for(i = 0; i < 3; i++)
		d[i] = p[i] - q[i];

	if(glm_vec_norm(d) > maxrange) {
		return NULL;
	} else {
		vec3 **list = calloc(2, sizeof(vec3*));

		list[0] = malloc(sizeof(vec3));
		for(i = 0; i < 3; i++)
			(*list[0])[i] = p[i];
		list[1] = NULL;

		return list;
	}
}

void skip_background_anim(Stage3D *s3d, void (*update_func)(void), int frames, int *timer, int *timer2) {
	// FIXME: This function is broken.
	// It must not modify the timers if frames <= 0.
	// Fix this when we no longer need v1.2 compat.

	int targetframes = *timer + frames;

	while(++(*timer) < targetframes) {
		if(timer2) {
			++(*timer2);
		}

		update_func();
	}

	if(timer2) {
		++(*timer2);
	}
}
