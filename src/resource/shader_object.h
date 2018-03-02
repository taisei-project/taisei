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

typedef enum ShaderObjectType {
	SHOBJ_VERT,
	SHOBJ_FRAG,
	SHOBJ_NUM_TYPES,
} ShaderObjectType;

typedef struct ShaderObjectImpl ShaderObjectImpl;

typedef struct ShaderObject {
	ShaderObjectType type;
	ShaderObjectImpl *impl;
} ShaderObject;

extern ResourceHandler shader_object_res_handler;

#define SHOBJ_PATH_PREFIX "res/shader/"
