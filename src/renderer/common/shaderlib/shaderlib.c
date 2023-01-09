/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "shaderlib.h"

void shader_free_source(ShaderSource *src) {
	switch(src->lang.lang) {
		case SHLANG_GLSL:
			glsl_free_source(src);
			break;

		default: break;
	}

	mem_free(src->content);
	src->content = NULL;
	src->content_size = 0;
}

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

bool shader_lang_supports_attribute_locations(const ShaderLangInfo *lang) {
	if(lang->lang == SHLANG_SPIRV) {
		return true;
	}

	if(lang->lang != SHLANG_GLSL) {
		return false; // FIXME?
	}

	if(lang->glsl.version.profile == GLSL_PROFILE_ES) {
		return lang->glsl.version.version >= 300;
	} else {
		return lang->glsl.version.version >= 330;
	}
}
