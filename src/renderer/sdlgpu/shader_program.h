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

#include "sdlgpu.h"

#include "memory/arena.h"

struct ShaderProgram {
	union {
		struct {
			ShaderObject *vertex;
			ShaderObject *fragment;
		};
		ShaderObject *as_array[2];
	} stages;
	MemArena arena;
	ht_str2ptr_t uniforms;
	sdlgpu_id_t id;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

struct Uniform {
	ShaderProgram *shader;

	// XXX: We have have to set uniforms separately per each stage.
	// We could've stored pointers to the relevant ShaderObjectUniform pointers here directly,
	// but that would required special handling in case of a dynamic shader reload.
	// Make it a stupid wrapper around the string key for now.
	const char *name;
	hash_t hash;
};

ShaderProgram *sdlgpu_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]);
void sdlgpu_shader_program_destroy(ShaderProgram *prog);
void sdlgpu_shader_program_set_debug_label(ShaderProgram *prog, const char *label);
const char *sdlgpu_shader_program_get_debug_label(ShaderProgram *prog);

Uniform *sdlgpu_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash);
UniformType sdlgpu_uniform_type(Uniform *uniform);
void sdlgpu_uniform(Uniform *uniform, uint offset, uint count, const void *data);
void sdlgpu_unref_texture_from_samplers(Texture *tex);
void sdlgpu_uniforms_handle_texture_pointer_renamed(Texture *pold, Texture *pnew);
bool sdlgpu_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src);
