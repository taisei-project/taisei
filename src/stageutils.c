/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stageutils.h"
#include <string.h>
#include <stdlib.h>
#include "taiseigl.h"
#include "global.h"

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
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	float facw = SCREEN_W/(float)VIEWPORT_W;
	float fach = SCREEN_H/(float)VIEWPORT_H;
	glScalef(facw,fach,1);
	gluPerspective(s->projangle, 1, n, f);
	glTranslatef(vx+vw/2.0, vy+vh/2.0, 0);

	glMatrixMode(GL_MODELVIEW);
}

void set_perspective(Stage3D *s, float n, float f) {
	set_perspective_viewport(s, n, f, VIEWPORT_X, VIEWPORT_Y, VIEWPORT_W, VIEWPORT_H);
}

void draw_stage3d(Stage3D *s, float maxrange) {
	int i,j;
	for(i = 0; i < 3;i++)
		s->cx[i] += s->cv[i];

	if(s->nodraw) {
		return;
	}

	glPushMatrix();

	if(s->crot[0])
		glRotatef(-s->crot[0], 1, 0, 0);
	if(s->crot[1])
		glRotatef(-s->crot[1], 0, 1, 0);
	if(s->crot[2])
		glRotatef(-s->crot[2], 0, 0, 1);

	if(s->cx[0] || s->cx[1] || s->cx[2])
		glTranslatef(-s->cx[0],-s->cx[1],-s->cx[2]);

	for(i = 0; i < s->msize; i++) {
		Vector **list;
		list = s->models[i].pos(s->cx, maxrange);

		for(j = 0; list && list[j] != NULL; j++) {
			s->models[i].draw(*list[j]);
			free(list[j]);
		}

		free(list);
	}

	glPopMatrix();
}

void free_stage3d(Stage3D *s) {
	free(s->models);
}

Vector **linear3dpos(Vector q, float maxrange, Vector p, Vector r) {
	int i;
	float n = 0, z = 0;
	for(i = 0; i < 3; i++) {
		n += q[i]*r[i] - p[i]*r[i];
		z += r[i]*r[i];
	}

	float t = n/z;

	Vector **list = NULL;
	int size = 0;
	int mod = 1;

	int num = t;
	while(1) {
		Vector dif;

		for(i = 0; i < 3; i++)
			dif[i] = q[i] - p[i] - r[i]*num;

		if(length(dif) < maxrange) {
			list = realloc(list, (++size)*sizeof(Vector*));
			list[size-1] = malloc(sizeof(Vector));
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

	list = realloc(list, (++size)*sizeof(Vector*));
	list[size-1] = NULL;

	return list;
}

Vector **single3dpos(Vector q, float maxrange, Vector p) {
	Vector d;

	int i;

	for(i = 0; i < 3; i++)
		d[i] = p[i] - q[i];

	if(length(d) > maxrange) {
		return NULL;
	} else {
		Vector **list = calloc(2, sizeof(Vector*));

		list[0] = malloc(sizeof(Vector));
		for(i = 0; i < 3; i++)
			(*list[0])[i] = p[i];
		list[1] = NULL;

		return list;
	}
}

void skip_background_anim(Stage3D *s3d, void (*drawfunc)(void), int frames, int *timer, int *timer2) {
	// two timers because stage 6 is so fucking special
	// second is optional

	if(s3d) {
		s3d->nodraw = true;
	}

	int targetframes = *timer + frames;

	while(++(*timer) < targetframes) {
		if(timer2) {
			++(*timer2);
		}
		drawfunc();
	}

	if(timer2) {
		++(*timer2);
	}

	if(s3d) {
		s3d->nodraw = false;
	}
}
