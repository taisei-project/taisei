/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shaderlib.h"

bool shader_lang_supports_uniform_locations(const ShaderLangInfo *lang) {
	if(lang->lang == SHLANG_SPIRV) {
		return true;
	}

	if(lang->lang != SHLANG_GLSL) {
		return false; // FIXME?
	}

	if(lang->glsl.version.profile == GLSL_PROFILE_ES) {
		return lang->glsl.version.version >= 310;
	} else {
		return lang->glsl.version.version >= 430;
	}
}

const char *shader_lang_name(ShaderLanguage lang) {
	switch(lang) {
		case SHLANG_INVALID: return "INVALID";
		case SHLANG_GLSL:    return "GLSL";
		case SHLANG_SPIRV:   return "SPIR-V";
		case SHLANG_HLSL:    return "HLSL";
		case SHLANG_DXBC:    return "DXBC";
		case SHLANG_MSL:     return "MSL";
	}

	assert(0 && "Unknown ShaderLanguage value");
	return "UNKNOWN";
}
