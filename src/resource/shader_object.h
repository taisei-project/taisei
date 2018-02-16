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
#include "taiseigl.h"

typedef enum ShaderObjectType {
	SHOBJ_VERT,
	SHOBJ_FRAG,
	SHOBJ_NUM_TYPES,
} ShaderObjectType;

typedef struct ShaderObject {
	ShaderObjectType type;
	GLuint gl_handle;
} ShaderObject;

char* shader_object_path(const char *name);
bool check_shader_object_path(const char *path);
void* load_shader_object_begin(const char *path, unsigned int flags);
void* load_shader_object_end(void *opaque, const char *path, unsigned int flags);
void unload_shader_object(void *vsha);

extern ResourceHandler shader_object_res_handler;

#define SHOBJ_PATH_PREFIX "res/shader/"
