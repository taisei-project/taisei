/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

typedef enum ShaderStage {
	SHADER_STAGE_INVALID,
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
} ShaderStage;

typedef enum ShaderLanguage {
	SHLANG_INVALID,
	SHLANG_GLSL,
} ShaderLanguage;

typedef struct ShaderSource ShaderSource;

#include "shader_glsl.h"

typedef struct ShaderLangInfo {
	ShaderLanguage lang;

	union {
		struct {
			GLSLVersion version;
		} glsl;
	};
} ShaderLangInfo;

struct ShaderSource {
	char *content;
	size_t content_size;
	ShaderLangInfo lang;
	ShaderStage stage;
};
