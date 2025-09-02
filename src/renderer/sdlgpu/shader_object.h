/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "sdlgpu.h"

#include "../api.h"
#include "../common/shaderlib/reflect.h"
#include "../common/magic_uniforms.h"

#include <SDL3/SDL_gpu.h>

typedef struct ShaderObjectUniform {
	UniformType type;

	union {
		struct {
			uint binding;
		} sampler;

		struct {
			uint offset;
			ShaderDataType type;
		} buffer_backed;
	};
} ShaderObjectUniform;

struct ShaderObject {
	MemArena arena;
	SDL_GPUShader *shader;

	struct {
		uint8_t *data;
		uint size;
		uint binding;
	} uniform_buffer;

	ht_str2ptr_t uniforms;
	ShaderObjectUniform *magic_unfiroms[NUM_MAGIC_UNIFORMS];
	Texture **sampler_bindings;
	uint64_t used_input_locations_map;
	uint num_sampler_bindings;

	ShaderStage stage;
	SDL_AtomicInt refs;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

bool sdlgpu_shader_language_supported(const ShaderLangInfo *lang, SPIRVTranspileOptions *transpile_opts);

ShaderObject *sdlgpu_shader_object_compile(ShaderSource *source);
void sdlgpu_shader_object_destroy(ShaderObject *shobj);
void sdlgpu_shader_object_set_debug_label(ShaderObject *shobj, const char *label);
const char *sdlgpu_shader_object_get_debug_label(ShaderObject *shobj);
bool sdlgpu_shader_object_transfer(ShaderObject *dst, ShaderObject *src);

ShaderObjectUniform *sdlgpu_shader_object_get_uniform(
	ShaderObject *shobj, const char *name, hash_t name_hash);
void sdlgpu_shader_object_uniform_set_data(
	ShaderObject *shobj, ShaderObjectUniform *uni,
	uint offset, uint count, const void *data);
bool sdlgpu_shader_object_uniform_types_compatible(ShaderObjectUniform *a, ShaderObjectUniform *b);
