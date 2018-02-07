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

#include <stdbool.h>

typedef int ivec3[3];

typedef struct ObjFileData {
	vec3 *xs;
	int xcount;

	vec3 *normals;
	int ncount;

	vec3 *texcoords;
	int tcount;

	ivec3 *indices;
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
