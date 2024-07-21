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

#include <SDL3/SDL_gpu.h>

struct ShaderObject {
	SDL_GpuShader *shader;
	ShaderStage stage;
	SDL_AtomicInt refs;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

bool sdlgpu_shader_language_supported(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative);

ShaderObject *sdlgpu_shader_object_compile(ShaderSource *source);
void sdlgpu_shader_object_destroy(ShaderObject *shobj);
void sdlgpu_shader_object_set_debug_label(ShaderObject *shobj, const char *label);
const char *sdlgpu_shader_object_get_debug_label(ShaderObject *shobj);
bool sdlgpu_shader_object_transfer(ShaderObject *dst, ShaderObject *src);
