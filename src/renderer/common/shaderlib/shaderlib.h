/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"
#include "cache.h"

#include "lang_glsl.h"
#include "lang_spirv.h"

struct ShaderLangInfo {
	ShaderLanguage lang;

	union {
		ShaderLangInfoGLSL glsl;
		ShaderLangInfoSPIRV spirv;
	};
};

union ShaderSourceMeta {
	ShaderSourceMetaGLSL glsl;
};

struct ShaderSource {
	char *content;
	size_t content_size;
	ShaderLangInfo lang;
	ShaderSourceMeta meta;
	ShaderStage stage;
};

void shader_free_source(ShaderSource *src);
bool shader_lang_supports_uniform_locations(const ShaderLangInfo *lang) attr_nonnull(1);
bool shader_lang_supports_attribute_locations(const ShaderLangInfo *lang) attr_nonnull(1);
