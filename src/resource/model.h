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

DEFINE_RESOURCE_GETTER(Model, res_model, RES_MODEL)
DEFINE_OPTIONAL_RESOURCE_GETTER(Model, res_model_optional, RES_MODEL)

extern ResourceHandler model_res_handler;

#endif // IGUARD_resource_model_h
