/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_object.h"

#include "renderer/common/shaderlib/reflect.h"
#include "sdlgpu.h"

#define UNIFORM_BLOCK_NAME "gl_DefaultUniformBlock"

INLINE SDL_GpuShaderStage shader_stage_ts2sdl(ShaderStage stage) {
	switch(stage) {
		case SHADER_STAGE_VERTEX:	return SDL_GPU_SHADERSTAGE_VERTEX;
		case SHADER_STAGE_FRAGMENT:	return SDL_GPU_SHADERSTAGE_FRAGMENT;
		default: UNREACHABLE;
	}
}

bool sdlgpu_shader_language_supported(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative) {
	if(lang->lang == SHLANG_SPIRV) {
		return true;
	}

	if(out_alternative) {
		*out_alternative = (ShaderLangInfo) {
			.lang = SHLANG_SPIRV,
			.spirv.target = SPIRV_TARGET_VULKAN_10,
		};
	}

	return false;
}

static ShaderObjectUniform *sdlgpu_shader_object_alloc_uniform(
	ShaderObject *shobj, const char *name, size_t extra
) {
	assert(!ht_lookup(&shobj->uniforms, name, NULL));

	auto uni = ARENA_ALLOC_FLEX(&shobj->arena, ShaderObjectUniform, extra);
	ht_set(&shobj->uniforms, name, uni);

	return NOT_NULL(uni);
}

static void sdlgpu_shader_object_add_buffer_backed_uniform(
	ShaderObject *shobj, const ShaderStructField *field
) {
	auto type = shader_type_to_uniform_type(&field->type);

	if(type == UNIFORM_UNKNOWN) {
		log_warn("Uniform %s has an unsupported type, ignoring", field->name);
		return;
	}

	log_debug("Added %s", field->name);

	*sdlgpu_shader_object_alloc_uniform(shobj, field->name, 0) = (ShaderObjectUniform) {
		.type = type,
		.buffer_backed = {
			.offset = field->offset,
			.type = field->type,
		},
	};
}

static UniformType sampler_type_to_uniform_type(const ShaderSamplerType *stype) {
	// The API only supports basic 2D and cubemap samplers for now

	if(stype->is_arrayed || stype->is_depth || stype->is_multisampled) {
		return UNIFORM_UNKNOWN;
	}

	if(stype->dim == SHADER_SAMPLER_DIM_2D) {
		return UNIFORM_SAMPLER_2D;
	}

	if(stype->dim == SHADER_SAMPLER_DIM_CUBE) {
		return UNIFORM_SAMPLER_CUBE;
	}

	return UNIFORM_UNKNOWN;
}

static void sdlgpu_shader_object_add_sampler_uniform(
	ShaderObject *shobj, const ShaderSampler *sampler
) {
	auto type = sampler_type_to_uniform_type(&sampler->type);

	if(type == UNIFORM_UNKNOWN) {
		log_warn("Uniform %s has an unsupported type, ignoring", sampler->name);
		return;
	}

	log_debug("Added %s", sampler->name);

	size_t bound_tex_storage = sampler->array_size * sizeof(Texture*);

	*sdlgpu_shader_object_alloc_uniform(shobj, sampler->name, bound_tex_storage) = (ShaderObjectUniform) {
		.type = type,
		.sampler = {
			.binding = sampler->binding,
			.array_size = sampler->array_size,
		},
	};
}

static void sdlgpu_shader_object_init_uniforms(ShaderObject *shobj, const ShaderReflection *reflection) {
	ht_create(&shobj->uniforms);
	marena_init(&shobj->arena, 32 * sizeof(ShaderObjectUniform));

	uint target_desc_set = shobj->stage == SHADER_STAGE_VERTEX ? 1 : 3;

	for(uint i = 0; i < reflection->num_uniform_buffers; ++i) {
		auto ubo = &reflection->uniform_buffers[i];

		if(ubo->set != target_desc_set) {
			log_debug("Uniform block %s (set=%u, binding=%u) ignored (wrong descriptor set, need %u)",
					  ubo->name, ubo->set, ubo->binding, target_desc_set);
			continue;
		}

		if(strcmp(UNIFORM_BLOCK_NAME, ubo->name)) {
			log_debug("Uniform block %s (set=%u, binding=%u) ignored (wrong name, need " UNIFORM_BLOCK_NAME ")",
					  ubo->name, ubo->set, ubo->binding);
			continue;
		}

		log_debug("Found " UNIFORM_BLOCK_NAME " (set=%u, binding=%u), %u bytes", ubo->set, ubo->binding, ubo->size);
		shobj->uniform_buffer.size = ubo->size;
		shobj->uniform_buffer.data = mem_alloc(ubo->size);
		shobj->uniform_buffer.binding = ubo->binding;

		for(uint f = 0; f < ubo->num_fields; ++f) {
			sdlgpu_shader_object_add_buffer_backed_uniform(shobj, &ubo->fields[f]);
		}
	}

	for(uint i = 0; i < reflection->num_samplers; ++i) {
		sdlgpu_shader_object_add_sampler_uniform(shobj, &reflection->samplers[i]);
	}
}

ShaderObject *sdlgpu_shader_object_compile(ShaderSource *source) {
	if(UNLIKELY(!sdlgpu_shader_language_supported(&source->lang, NULL))) {
		log_error("Shading language not supported");
		return NULL;
	}

	MemArena reflect_arena;
	marena_init(&reflect_arena, 1 << 14);

	auto reflection = shader_source_reflect(source, &reflect_arena);

	if(!reflection) {
		log_error("Shader reflection failed");
		marena_deinit(&reflect_arena);
		return NULL;
	}

	SDL_GpuShader *shader = SDL_GpuCreateShader(sdlgpu.device, &(SDL_GpuShaderCreateInfo) {
		.stage = shader_stage_ts2sdl(source->stage),
		.code = (uint8_t*)source->content,
		.codeSize = source->content_size - 1,  // content always contains an extra NULL byte
		.format = SDL_GPU_SHADERFORMAT_SPIRV,

		// FIXME
		.entryPointName = "main",
		.samplerCount = reflection->num_samplers,
		.storageTextureCount = 0,
		.storageBufferCount = 0,
		.uniformBufferCount = reflection->num_uniform_buffers,
	});

	if(UNLIKELY(!shader)) {
		log_sdl_error(LOG_ERROR, "SDL_GpuCreateShader");
		return NULL;
	}

	auto shobj = ALLOC(ShaderObject, {
		.shader = shader,
		.stage = source->stage,
		.refs.value = 1,
	});

	sdlgpu_shader_object_init_uniforms(shobj, reflection);
	marena_deinit(&reflect_arena);

	return shobj;
}

static void sdlgpu_shader_object_free_suballocations(ShaderObject *shobj) {
	ht_destroy(&shobj->uniforms);
	marena_deinit(&shobj->arena);
	mem_free(shobj->uniform_buffer.data);
}

void sdlgpu_shader_object_destroy(ShaderObject *shobj) {
	if(SDL_AtomicDecRef(&shobj->refs)) {
		SDL_GpuReleaseShader(sdlgpu.device, shobj->shader);
		sdlgpu_shader_object_free_suballocations(shobj);
		mem_free(shobj);
	}
}

void sdlgpu_shader_object_set_debug_label(ShaderObject *shobj, const char *label) {
	strlcpy(shobj->debug_label, label, sizeof(shobj->debug_label));
}

const char *sdlgpu_shader_object_get_debug_label(ShaderObject *shobj) {
	return shobj->debug_label;
}

bool sdlgpu_shader_object_transfer(ShaderObject *dst, ShaderObject *src) {
	sdlgpu_shader_object_free_suballocations(dst);
	src->refs = dst->refs;

	SDL_GpuReleaseShader(sdlgpu.device, dst->shader);
	*dst = *src;

	return true;
}

INLINE bool is_sampler_uniform(UniformType utype) {
	return utype == UNIFORM_SAMPLER_2D || utype == UNIFORM_SAMPLER_CUBE;
}

ShaderObjectUniform *sdlgpu_shader_object_get_uniform(
	ShaderObject *shobj, const char *name, hash_t name_hash
) {
	return ht_get_prehashed(&shobj->uniforms, name, name_hash, NULL);
}

void sdlgpu_shader_object_uniform_set_data(
	ShaderObject *shobj, ShaderObjectUniform *uni,
	uint offset, uint count, const void *data
) {
	if(is_sampler_uniform(uni->type)) {
		assert(offset + count <= uni->sampler.array_size);
		Texture *const *textures = data;
		memcpy(uni->sampler.bound_textures + offset, textures, sizeof(*textures) * count);
	} else {
		// FIXME no source buffer size supplied by the API, infer it.
		auto uti = r_uniform_type_info(uni->type);
		uint src_size = uti->elements * uti->element_size * count;

		uint field_size = shader_type_size(&uni->buffer_backed.type);
		uint in_field_offset = 0;

		if(uni->buffer_backed.type.array_stride > 1) {
			in_field_offset = uni->buffer_backed.type.array_stride * offset;
			assert(field_size >= in_field_offset);
			field_size -= in_field_offset;
		} else {
			assert(offset == 0);
			assert(count == 1);
		}

		uint8_t *dst = shobj->uniform_buffer.data + uni->buffer_backed.offset + in_field_offset;

		attr_unused uint num_copied = shader_type_unpack_from_bytes(
			&uni->buffer_backed.type,
			src_size, data,
			field_size, dst
		);

		assert(num_copied == count);
	}
}

bool sdlgpu_shader_object_uniform_types_compatible(ShaderObjectUniform *a, ShaderObjectUniform *b) {
	if(a->type != b->type) {
		return false;
	}

	if(is_sampler_uniform(a->type)) {
		return a->sampler.array_size == b->sampler.array_size;
	}

	return !memcmp(&a->buffer_backed.type, &b->buffer_backed.type, sizeof(a->buffer_backed.type));
}