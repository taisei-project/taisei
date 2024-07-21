/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_object.h"

#include "sdlgpu.h"

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

ShaderObject *sdlgpu_shader_object_compile(ShaderSource *source) {
	if(UNLIKELY(!sdlgpu_shader_language_supported(&source->lang, NULL))) {
		log_error("Shading language not supported");
		return NULL;
	}

	SDL_GpuShader *shader = SDL_GpuCreateShader(sdlgpu.device, &(SDL_GpuShaderCreateInfo) {
		.stage = shader_stage_ts2sdl(source->stage),
		.code = (uint8_t*)source->content,
		.codeSize = source->content_size - 1,  // content always contains an extra NULL byte
		.format = SDL_GPU_SHADERFORMAT_SPIRV,

		// FIXME
		.entryPointName = "main",
		.samplerCount = 0,
		.storageTextureCount = 0,
		.storageBufferCount = 0,
		.uniformBufferCount = 0,
	});

	if(UNLIKELY(!shader)) {
		log_sdl_error(LOG_ERROR, "SDL_GpuCreateShader");
		return NULL;
	}

	return ALLOC(ShaderObject, {
		.shader = shader,
		.stage = source->stage,
		.refs.value = 1,
	});
}

void sdlgpu_shader_object_destroy(ShaderObject *shobj) {
	if(SDL_AtomicDecRef(&shobj->refs)) {
		SDL_GpuReleaseShader(sdlgpu.device, shobj->shader);
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
	return false;
}
