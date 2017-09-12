/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
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

char* model_path(const char *name);
bool check_model_path(const char *path);
void* load_model_begin(const char *path, unsigned int flags);
void* load_model_end(void *opaque, const char *path, unsigned int flags);
void unload_model(void*); // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space

Model* get_model(const char *name);

void draw_model_p(Model *model);
void draw_model(const char *name);

#define MDL_PATH_PREFIX "res/models/"
#define MDL_EXTENSION ".obj"

#endif
