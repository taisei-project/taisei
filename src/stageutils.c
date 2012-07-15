/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stageutils.h"
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include "global.h"

void init_stage3d(Stage3D *s) {
	memset(s, 0, sizeof(Stage3D));
	s->projangle = 45;
}

void add_model(Stage3D *s, SegmentDrawRule draw, SegmentPositionRule pos) {
	s->models = realloc(s->models, (++s->msize)*sizeof(StageSegment));
	
	s->models[s->msize - 1].draw = draw;
	s->models[s->msize - 1].pos = pos;
}

void set_perspective(Stage3D *s, float near, float far) {
	glMatrixMode(GL_PROJECTION);
	
	glLoadIdentity();
	glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0)/SCREEN_W, -(VIEWPORT_Y+VIEWPORT_H/2.0)/SCREEN_H,0);
	gluPerspective(s->projangle, 1, near, far);
	
	if(s->crot[0])
		glRotatef(s->crot[0], 1, 0, 0);
	if(s->crot[1])
		glRotatef(s->crot[1], 0, 1, 0);
	if(s->crot[2])
		glRotatef(s->crot[2], 0, 0, 1);
	
	if(s->cx[0] || s->cx[1] || s->cx[2])
		glTranslatef(s->cx[0],s->cx[1],s->cx[2]);
	
	glMatrixMode(GL_MODELVIEW);	
}

void draw_stage3d(Stage3D *s, float maxrange) {
	int i,j;
	for(i = 0; i < 3;i++)
		s->cx[i] += s->cv[i];
	
	for(i = 0; i < s->msize; i++) {
		Vector **list;
		list = s->models[i].pos(s->cx, maxrange);
		
		for(j = 0; list[j] != NULL; j++) {
			s->models[i].draw(*list[j]);
			free(list[j]);
		}
		
		free(list);
	}
}

void free_stage3d(Stage3D *s) {
	if(s->models != NULL)
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
				(*list[size-1])[i] = - p[i] - r[i]*num;
		} else if(mod == 1) {
			mod = -1;
			num = t;
		} else {
			break;
		}
		
		num += mod;
	}
		
	list = realloc(list, (++size)*sizeof(Vector));
	list[size-1] = NULL;
	
	return list;
}
