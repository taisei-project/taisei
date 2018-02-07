/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"

typedef struct StageSegment StageSegment;

typedef void (*SegmentDrawRule)(vec3 pos);
typedef vec3 **(*SegmentPositionRule)(vec3 q, float maxrange); // returns NULL-terminated array

struct StageSegment {
	SegmentDrawRule draw;
	SegmentPositionRule pos;
};

typedef struct Stage3D Stage3D;
struct Stage3D {
	StageSegment *models;
	int msize;

	// Camera
	vec3 cx; // x
	vec3 cv; // x'

	vec3 crot;

	float projangle;
};

extern Stage3D stage_3d_context;

void init_stage3d(Stage3D *s);

void add_model(Stage3D *s, SegmentDrawRule draw, SegmentPositionRule pos);

void set_perspective_viewport(Stage3D *s, float n, float f, int vx, int vy, int vw, int vh);
void set_perspective(Stage3D *s, float near, float far);
void draw_stage3d(Stage3D *s, float maxrange);
void update_stage3d(Stage3D *s);

void free_stage3d(Stage3D *s);

vec3 **linear3dpos(vec3 q, float maxrange, vec3 p, vec3 r);

vec3 **single3dpos(vec3 q, float maxrange, vec3 p);

void skip_background_anim(Stage3D *s3d, void (*update_func)(void), int frames, int *timer, int *timer2);
