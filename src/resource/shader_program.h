/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"

typedef struct ShaderProgram ShaderProgram;

extern ResourceHandler shader_program_res_handler;

#define SHPROG_PATH_PREFIX "res/shader/"
#define SHPROG_EXT ".prog"

DEFINE_RESOURCE_GETTER(ShaderProgram, res_shader, RES_SHADER_PROGRAM)
DEFINE_OPTIONAL_RESOURCE_GETTER(ShaderProgram, res_shader_optional, RES_SHADER_PROGRAM)
