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

typedef enum ShaderStage {
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
} ShaderStage;

typedef struct ShaderObjectImpl ShaderObjectImpl;

typedef struct ShaderObject {
	ShaderStage stage;
	ShaderObjectImpl *impl;
} ShaderObject;

#define SHOBJ_PATH_PREFIX "res/shader/"
