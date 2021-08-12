/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "hashtable.h"
#include "../api.h"
#include "opengl.h"
#include "resource/shader_program.h"

typedef enum MagicUniformIndex {
	UMAGIC_INVALID = -1,

	UMAGIC_MATRIX_MV,
	UMAGIC_MATRIX_PROJ,
	UMAGIC_MATRIX_TEX,
	UMAGIC_COLOR,
	UMAGIC_VIEWPORT,
	UMAGIC_COLOR_OUT_SIZES,
	UMAGIC_DEPTH_OUT_SIZE,

	NUM_MAGIC_UNIFORMS,
} MagicUniformIndex;

struct ShaderProgram {
	GLuint gl_handle;
	ht_str2ptr_t uniforms;
	Uniform *magic_uniforms[NUM_MAGIC_UNIFORMS];
	char debug_label[R_DEBUG_LABEL_SIZE];
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
