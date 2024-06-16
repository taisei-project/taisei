/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"

typedef struct ShaderObject ShaderObject;

extern ResourceHandler shader_object_res_handler;

#define SHOBJ_PATH_PREFIX "res/shader/"

DEFINE_RESOURCE_GETTER(ShaderObject, res_shader_object, RES_SHADER_OBJECT)
DEFINE_OPTIONAL_RESOURCE_GETTER(ShaderObject, res_shader_object_optional, RES_SHADER_OBJECT)
