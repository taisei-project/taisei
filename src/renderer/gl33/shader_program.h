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

struct Uniform {
	// these are for sampler uniforms
	LIST_INTERFACE(Uniform);
	Texture **textures;

	ShaderProgram *prog;
	size_t elem_size; // bytes
	uint array_size; // elements
	uint location;
	UniformType type;

	struct {
		// buffer size = elem_size * array_size
		char *pending;
		char *commited;

		uint update_first_idx;
		uint update_last_idx;
	} cache;
};

void gl33_sync_uniforms(ShaderProgram *prog);

Uniform* gl33_shader_uniform(ShaderProgram *prog, const char *uniform_name);
UniformType gl33_uniform_type(Uniform *uniform);
void gl33_uniform(Uniform *uniform, uint offset, uint count, const void *data);
void gl33_unref_texture_from_samplers(Texture *tex);

extern ResourceHandler gl33_shader_program_res_handler;
