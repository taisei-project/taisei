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
#include "../api.h"
#include "opengl.h"
#include "resource/resource.h"
#include "resource/shader_program.h"

struct ShaderProgram {
	GLuint gl_handle;
	ht_str2ptr_t uniforms;
};

typedef void (*UniformSetter)(Uniform *uniform, uint count, const void *data);

typedef struct UniformCache {
	void *data;
	size_t size;
} UniformCache;

struct Uniform {
	ShaderProgram *prog;
	uint location;
	UniformType type;

	struct {
		UniformCache pending;
		UniformCache commited;
		uint update_count;
		size_t update_size;
	} cache;
};

void gl33_sync_uniforms(ShaderProgram *prog);

Uniform* gl33_shader_uniform(ShaderProgram *prog, const char *uniform_name);
UniformType gl33_uniform_type(Uniform *uniform);
void gl33_uniform(Uniform *uniform, uint count, const void *data);

extern ResourceHandler gl33_shader_program_res_handler;
