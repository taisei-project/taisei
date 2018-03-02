/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"

typedef struct ShaderProgram ShaderProgram;

extern ResourceHandler shader_program_res_handler;

#define SHPROG_PATH_PREFIX "res/shader/"
#define SHPROG_EXT ".prog"
