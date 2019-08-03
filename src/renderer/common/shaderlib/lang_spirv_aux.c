/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "shaderlib.h"
#include "lang_spirv_private.h"
#include "util.h"

static bool shader_cache_entry_name(const ShaderLangInfo *lang, ShaderStage stage, SPIRVOptimizationLevel optlvl, char *buf, size_t bufsize) {
	// TODO: somehow get rid of this ugly optlvl parameter and generalize external metadata?

	switch(lang->lang) {
		case SHLANG_GLSL: {
			snprintf(buf, bufsize, "glsl_%u_%u_%u", stage, lang->glsl.version.version, lang->glsl.version.profile);
			return true;
		}

		case SHLANG_SPIRV: {
			snprintf(buf, bufsize, "spirv_%u_%u_%u", stage, lang->spirv.target, optlvl);
			return true;
		}

		default: {
			log_warn("Unhandled shading language id=%u", lang->lang);
			return false;
		}
	}

	return false;
}

bool spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	ShaderLangInfo target_lang = { 0 };
	target_lang.lang = SHLANG_SPIRV;
	target_lang.spirv.target = options->target;

	if(!shader_cache_entry_name(&target_lang, in->stage, options->optimization_level, name, sizeof(name))) {
		return _spirv_compile(in, out, options);
	}

	if(!shader_cache_hash(in, hash, sizeof(hash))) {
		return _spirv_compile(in, out, options);
	}

	bool result;

	if((result = shader_cache_get(hash, name, out))) {
		if(
			!memcmp(&target_lang, &out->lang, sizeof(ShaderLangInfo)) &&
			out->stage == in->stage
		) {
			return true;
		}

		log_warn("Invalid cache entry ignored");
	}

	if((result = _spirv_compile(in, out, options))) {
		shader_cache_set(hash, name, out);
	}

	return result;
}

bool spirv_decompile(const ShaderSource *in, ShaderSource *out, const SPIRVDecompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	if(!shader_cache_entry_name(options->lang, in->stage, 0, name, sizeof(name))) {
		return _spirv_decompile(in, out, options);
	}

	if(!shader_cache_hash(in, hash, sizeof(hash))) {
		return _spirv_decompile(in, out, options);
	}

	bool result;

	if((result = shader_cache_get(hash, name, out))) {
		if(
			!memcmp(options->lang, &out->lang, sizeof(ShaderLangInfo)) &&
			out->stage == in->stage
		) {
			return true;
		}

		log_warn("Invalid cache entry ignored");
	}

	if((result = _spirv_decompile(in, out, options))) {
		shader_cache_set(hash, name, out);
	}

	return result;
}

static bool lang_supports_uniform_locations(const ShaderLangInfo *lang) {
	if(lang->lang != SHLANG_GLSL) {
		return false; // FIXME?
	}

	if(lang->glsl.version.profile == GLSL_PROFILE_ES) {
		return lang->glsl.version.version >= 310;
	} else {
		return lang->glsl.version.version >= 430;
	}
}

bool spirv_transpile(const ShaderSource *in, ShaderSource *out, const SPIRVTranspileOptions *options) {
	ShaderSource spirv = { 0 };
	bool result;

	ShaderSource _in = *in;
	in = &_in;

	if(
		!lang_supports_uniform_locations(&_in.lang) &&
		lang_supports_uniform_locations(options->lang) &&
		_in.lang.lang == SHLANG_GLSL
	) {
		// HACK: This is annoying... shaderc/glslang does not support GL_ARB_explicit_uniform_location
		// for some reason. Until there's a better solution, we'll try to compile the shader using a
		// higher version.

		if(_in.lang.glsl.version.profile == GLSL_PROFILE_ES) {
			_in.lang.glsl.version.version = 310;
		} else {
			_in.lang.glsl.version.version = 430;
		}
	}

	result = spirv_compile(in, &spirv, &(SPIRVCompileOptions) {
		.target = SPIRV_TARGET_OPENGL_450, // TODO: specify this in the shader
		.optimization_level = options->optimization_level,
		.filename = options->filename,

		// Preserve names of declarations.
		// This can be vital for shader interface matching.
		.debug_info = true,
	});

	if(result) {
		result = spirv_decompile(&spirv, out, &(SPIRVDecompileOptions) {
			.lang = options->lang,
		});

		if(result) {
			log_debug("%s: translated code:\n%s", options->filename, out->content);
		}
	}

	free(spirv.content);
	return result;
}
