/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_renderer_common_shader_defs_h
#define IGUARD_renderer_common_shader_defs_h

#include "taisei.h"

typedef enum ShaderStage {
	SHADER_STAGE_INVALID,
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
} ShaderStage;

typedef enum ShaderLanguage {
	SHLANG_INVALID,
	SHLANG_GLSL,
	SHLANG_SPIRV,
} ShaderLanguage;

typedef struct ShaderSource ShaderSource;
typedef struct ShaderLangInfo ShaderLangInfo;

#endif // IGUARD_renderer_common_shader_defs_h
