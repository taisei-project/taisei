/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stageutils_h
#define IGUARD_stageutils_h

#include "taisei.h"

#include "util.h"

typedef struct StageSegment StageSegment;
typedef struct Stage3D Stage3D;

typedef void (*SegmentDrawRule)(vec3 pos);
typedef uint (*SegmentPositionRule)(Stage3D *s3d, vec3 q, float maxrange); // returns number of elements written to Stage3D pos_buffer

struct StageSegment {
	SegmentDrawRule draw;
	SegmentPositionRule pos;
};

struct Stage3D {
	StageSegment *models;
	int msize;

	vec3 *pos_buffer;
	uint pos_buffer_size;

	// Camera
	vec3 cx; // x
	vec3 cv; // x'

	vec3 crot;

	float projangle;
};

extern Stage3D stage_3d_context;

void init_stage3d(Stage3D *s, uint pos_buffer_size);

void add_model(Stage3D *s, SegmentDrawRule draw, SegmentPositionRule pos);

void set_perspective_viewport(Stage3D *s, float n, float f, int vx, int vy, int vw, int vh);
void set_perspective(Stage3D *s, float near, float far);
void draw_stage3d(Stage3D *s, float maxrange);
void update_stage3d(Stage3D *s);

void free_stage3d(Stage3D *s);

uint linear3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p, vec3 r);
uint single3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p);

void skip_background_anim(void (*update_func)(void), int frames, int *timer, int *timer2);

#endif // IGUARD_stageutils_h
