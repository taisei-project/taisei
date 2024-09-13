/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"  // IWYU prama: export

#include "lang_glsl.h"
#include "lang_spirv.h"
#include "lang_hlsl.h"
#include "lang_dxbc.h"

struct ShaderLangInfo {
	ShaderLanguage lang;

	union {
		ShaderLangInfoGLSL glsl;
		ShaderLangInfoSPIRV spirv;
		ShaderLangInfoHLSL hlsl;
		ShaderLangInfoDXBC dxbc;
	};
};

struct ShaderSource {
	const char *content;
	const ShaderReflection *reflection;
	const char *entrypoint;
	size_t content_size;
	ShaderLangInfo lang;
	ShaderStage stage;
};

bool shader_lang_supports_uniform_locations(const ShaderLangInfo *lang) attr_nonnull(1);

const char *shader_lang_name(ShaderLanguage lang);
