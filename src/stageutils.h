/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGEUTILS_H
#define STAGEUTILS_H

#include "matrix.h"

typedef struct StageSegment StageSegment;

typedef void (*SegmentDrawRule)(Vector pos);
typedef Vector **(*SegmentPositionRule)(Vector q, float maxrange); // returns NULL-terminated array

struct StageSegment {
	SegmentDrawRule draw;
	SegmentPositionRule pos;
};

typedef struct Stage3D Stage3D;
struct Stage3D {
	StageSegment *models;
	int msize;
	
	// Camera
	Vector cx; // x
	Vector cv; // x'
	
	Vector crot;
	
	float projangle;
};

void init_stage3d(Stage3D *s);

void add_model(Stage3D *s, SegmentDrawRule draw, SegmentPositionRule pos);

void set_perspective_viewport(Stage3D *s, float n, float f, int vx, int vy, int vw, int vh);
void set_perspective(Stage3D *s, float near, float far);
void draw_stage3d(Stage3D *s, float maxrange);

void free_stage3d(Stage3D *s);

Vector **linear3dpos(Vector q, float maxrange, Vector p, Vector r);

Vector **single3dpos(Vector q, float maxrange, Vector p);
#endif
