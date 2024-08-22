/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "../common/magic_uniforms.h"
#include "opengl.h"

#include "hashtable.h"
#include "resource/shader_program.h"

struct ShaderProgram {
	GLuint gl_handle;
	ht_str2ptr_t uniforms;
	Uniform *magic_uniforms[NUM_MAGIC_UNIFORMS];
	char debug_label[R_DEBUG_LABEL_SIZE];
};

#define INVALID_UNIFORM_LOCATION 0xffffffff

struct Uniform {
	// these are for sampler uniforms
	LIST_INTERFACE(Uniform);
	Texture **textures;

	ShaderProgram *prog;
	size_t elem_size; // bytes
	uint array_size; // elements
	uint location;
	UniformType type;

	// corresponding _SIZE uniform (for samplers; optional)
	Uniform *size_uniform;

	struct {
		// buffer size = elem_size * array_size
		char *pending;
		char *commited;

		uint update_first_idx;
		uint update_last_idx;
	} cache;
};

void gl33_sync_uniforms(ShaderProgram *prog);

ShaderProgram *gl33_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]);
void gl33_shader_program_destroy(ShaderProgram *prog);
void gl33_shader_program_set_debug_label(ShaderProgram *prog, const char *label);
const char* gl33_shader_program_get_debug_label(ShaderProgram *prog);

Uniform *gl33_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash);
UniformType gl33_uniform_type(Uniform *uniform);
void gl33_uniform(Uniform *uniform, uint offset, uint count, const void *data);
void gl33_unref_texture_from_samplers(Texture *tex);
void gl33_uniforms_handle_texture_pointer_renamed(Texture *pold, Texture *pnew);
bool gl33_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src);
