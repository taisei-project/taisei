/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_spirv_private.h"
#include "shaderlib.h"
#include "cache.h"

#include "log.h"
#include "util.h"

static bool shader_cache_entry_name(
	const ShaderLangInfo *lang,
	ShaderStage stage,
	SPIRVOptimizationLevel optlvl,
	size_t bufsize,
	char buf[bufsize]
) {
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
			log_error("Unhandled shading language id=%u", lang->lang);
			return false;
		}
	}
}

bool spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	ShaderLangInfo target_lang = { 0 };
	target_lang.lang = SHLANG_SPIRV;
	target_lang.spirv.target = options->target;

	if(!shader_cache_entry_name(&target_lang, in->stage, options->optimization_level, sizeof(name), name)) {
		return _spirv_compile(in, out, options);
	}

	if(!shader_cache_hash(in, options->macros, sizeof(hash), hash)) {
		return _spirv_compile(in, out, options);
	}

	if(shader_cache_get(hash, name, out)) {
		if(
			!memcmp(&target_lang, &out->lang, sizeof(ShaderLangInfo)) &&
			out->stage == in->stage
		) {
			return true;
		}

		log_warn("Invalid cache entry ignored");
	}

	if(_spirv_compile(in, out, options)) {
		shader_cache_set(hash, name, out);
		return true;
	}

	return false;
}

bool spirv_decompile(const ShaderSource *in, ShaderSource *out, const SPIRVDecompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	if(!shader_cache_entry_name(options->lang, in->stage, 0, sizeof(name), name)) {
		return _spirv_decompile(in, out, options);
	}

	if(!shader_cache_hash(in, NULL, sizeof(hash), hash)) {
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

bool spirv_transpile(const ShaderSource *in, ShaderSource *out, const SPIRVTranspileOptions *options) {
	ShaderSource spirv = { 0 };
	bool result;

	ShaderSource _in = *in;
	in = &_in;

	if(
		!shader_lang_supports_uniform_locations(&_in.lang) &&
		// shader_lang_supports_uniform_locations(options->lang) &&
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

	char macro_buf[32];
	char *macro_buf_p = macro_buf;
	char *macro_buf_end = macro_buf + ARRAY_SIZE(macro_buf);
	ShaderMacro macros[16];
	int i = 0;

	#define ADD_MACRO(...) do { \
		assert(i < ARRAY_SIZE(macros)); \
		macros[i++] = (ShaderMacro) { __VA_ARGS__ }; \
		if(macros[i-1].name) { \
			log_debug("#define %s %s", macros[i-1].name, macros[i-1].value); \
		} \
	} while(0)

	#define ADD_MACRO_DYNAMIC(name, ...) do { \
		attr_unused int remaining = macro_buf_end - macro_buf_p; \
		int len = snprintf(macro_buf_p, remaining, __VA_ARGS__); \
		assert(len >= 0); \
		ADD_MACRO(name, macro_buf_p); \
		macro_buf_p += len + 1; \
		assert(macro_buf_p < macro_buf_end); \
	} while(0)

	#define ADD_MACRO_BOOL(name, value) \
		ADD_MACRO(name, (value) ? "1" : "0")

	ADD_MACRO_DYNAMIC("LANG_GLSL", "%d", SHLANG_GLSL);
	ADD_MACRO_DYNAMIC("LANG_SPIRV", "%d", SHLANG_SPIRV);
	ADD_MACRO_DYNAMIC("TRANSPILE_TARGET_LANG", "%d", options->lang->lang);

	switch(options->lang->lang) {
		case SHLANG_GLSL:
			ADD_MACRO_BOOL("TRANSPILE_TARGET_GLSL_ES", options->lang->glsl.version.profile == GLSL_PROFILE_ES);
			ADD_MACRO_DYNAMIC("TRANSPILE_TARGET_GLSL_VERSION", "%d", options->lang->glsl.version.version);
			break;

		case SHLANG_SPIRV:
			break;

		default: UNREACHABLE;
	}

	ADD_MACRO(NULL);

	SPIRVTarget target = SPIRV_TARGET_OPENGL_450;

	if(options->lang->lang == SHLANG_SPIRV) {
		target = options->lang->spirv.target;
	}

	result = spirv_compile(in, &spirv, &(SPIRVCompileOptions) {
		.target = target,
		.optimization_level = options->optimization_level,
		.filename = options->filename,
		.macros = macros,

		// Preserve names of declarations.
		// This can be vital for shader interface matching.
		.debug_info = true,
	});

	if(result) {
		if(options->lang->lang == SHLANG_SPIRV) {
			*out = spirv;
			return result;
		}

		result = spirv_decompile(&spirv, out, &(SPIRVDecompileOptions) {
			.lang = options->lang,
		});

		if(result) {
			log_debug("%s: translated code:\n%s", options->filename, out->content);
		}
	}

	mem_free(spirv.content);
	return result;
}
