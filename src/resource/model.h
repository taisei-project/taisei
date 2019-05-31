/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_model_h
#define IGUARD_resource_model_h

#include "taisei.h"

#include "util.h"
#include "resource.h"

#include "renderer/api.h"

typedef struct ObjFileData {
	vec3_noalign *xs;
	int xcount;

	vec3_noalign *normals;
	int ncount;

	vec3_noalign *texcoords;
	int tcount;

	ivec3_noalign *indices;
	int icount;

	int fverts;
} ObjFileData;

struct Model {
	VertexArray *vertex_array;
	size_t num_vertices;
	size_t offset;
	Primitive primitive;
	bool indexed;
};

Model* get_model(const char *name);

extern ResourceHandler model_res_handler;

#define MDL_EXTENSION ".obj"

#endif // IGUARD_resource_model_h
