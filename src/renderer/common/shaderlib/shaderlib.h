/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_renderer_common_shaderlib_shaderlib_h
#define IGUARD_renderer_common_shaderlib_shaderlib_h

#include "taisei.h"

#include "defs.h"
#include "cache.h"

#include "lang_glsl.h"
#include "lang_spirv.h"

struct ShaderLangInfo {
	ShaderLanguage lang;

	union {
		struct {
			GLSLVersion version;
		} glsl;

		struct {
			SPIRVTarget target;
		} spirv;
	};
};

struct ShaderSource {
	char *content;
	size_t content_size;
	ShaderLangInfo lang;
	ShaderStage stage;
};

#endif // IGUARD_renderer_common_shaderlib_shaderlib_h
