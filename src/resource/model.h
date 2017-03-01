/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MODEL_H
#define MODEL_H

#include "matrix.h"

typedef int IVector[3];

typedef struct ObjFileData {
	Vector *xs;
	int xcount;

	Vector *normals;
	int ncount;

	Vector *texcoords;
	int tcount;

	IVector *indices;
	int icount;

	int fverts;
} ObjFileData;

typedef struct Model {
	unsigned int *indices;
	int icount;
	int fverts;
} Model;

Model *load_model(const char *filename);
Model *get_model(const char *name);

void draw_model_p(Model *model);
void draw_model(const char *name);
void delete_models(void); // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space

#endif
