/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGEUITLS_H
#define STAGEUTILS_H

#include "matrix.h"

typedef struct Model Model;

typedef void (*ModelDrawRule)(Vector pos);
typedef Vector **(*ModelPositionRule)(Vector q, float maxrange); // returns NULL-terminated array

struct Model {
	ModelDrawRule draw;
	ModelPositionRule pos;
};

typedef struct Stage3D Stage3D;
struct Stage3D {
	Model *models;
	int msize;
	
	// Camera
	Vector cx; // x
	Vector cv; // x'
	
	Vector crot;
	
	float projangle;
};

void init_stage3d(Stage3D *s);

void add_model(Stage3D *s, ModelDrawRule draw, ModelPositionRule pos);

void set_perspective(Stage3D *s, float near, float far);
void draw_stage3d(Stage3D *s, float maxrange);

void free_stage3d(Stage3D *s);

Vector **linear3dpos(Vector q, float maxrange, Vector p, Vector r);
#endif