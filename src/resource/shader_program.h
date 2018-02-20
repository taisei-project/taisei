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
#include "hashtable.h"
#include "taiseigl.h"
#include "resource.h"

typedef struct ShaderProgram {
	GLuint gl_handle;
	Hashtable *uniforms;
	int renderctx_block_idx;
} ShaderProgram;

char* shader_program_path(const char *name);
bool check_shader_program_path(const char *path);
void* load_shader_program_begin(const char *path, unsigned int flags);
void* load_shader_program_end(void *opaque, const char *path, unsigned int flags);
void unload_shader_program(void *vsha);

ShaderProgram* get_shader_program(const char *name);
ShaderProgram* get_shader_program_optional(const char *name);

int uniloc(ShaderProgram *prog, const char *name);

extern ResourceHandler shader_program_res_handler;

#define SHPROG_PATH_PREFIX "res/shader/"
#define SHPROG_EXT ".prog"
