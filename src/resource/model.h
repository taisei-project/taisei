/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_model_h
#define IGUARD_resource_model_h

#include "taisei.h"

#include "resource.h"
#include "renderer/api.h"

struct Model {
	VertexArray *vertex_array;
	size_t num_vertices;
	size_t num_indices;
	size_t offset;
	Primitive primitive;
};

INLINE Model *get_model(const char *name) {
	return get_resource(RES_MODEL, name, RESF_DEFAULT)->data;
}

extern ResourceHandler model_res_handler;

#endif // IGUARD_resource_model_h
