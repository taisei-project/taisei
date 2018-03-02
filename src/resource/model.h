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
#include "resource.h"

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
	uint *indices;
	int icount;
	int fverts;
} Model;

char* model_path(const char *name);
bool check_model_path(const char *path);
void* load_model_begin(const char *path, uint flags);
void* load_model_end(void *opaque, const char *path, uint flags);
void unload_model(void*); // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space

Model* get_model(const char *name);

extern ResourceHandler model_res_handler;

#define MDL_PATH_PREFIX "res/models/"
#define MDL_EXTENSION ".obj"
