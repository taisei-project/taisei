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
#include "util/stringops.h"

typedef struct CacheEntryMetadata {
	ShaderLangInfo lang;
	ShaderStage stage;

	union {
		struct {
			const SPIRVCompileOptions *compile_options;
		} spirv;

		struct {
		} glsl;
	};
} CacheEntryMetadata;

static size_t shader_cache_serialize_entry_metadata_glsl(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->lang.glsl.version.version / 10,
		meta->lang.glsl.version.profile,
	};

	assume(sizeof(data) <= size);
	memcpy(buf, data, sizeof(data));
	return sizeof(data);
}

static size_t shader_cache_serialize_entry_metadata_spirv(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->lang.spirv.target,
		meta->spirv.compile_options->optimization_level,
		meta->spirv.compile_options->flags,
	};

	assume(sizeof(data) <= size);
	memcpy(buf, data, sizeof(data));
	return sizeof(data);
}

static size_t shader_cache_serialize_entry_metadata(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {

	switch(meta->lang.lang) {
		case SHLANG_GLSL:
			return shader_cache_serialize_entry_metadata_glsl(meta, size, buf);

		case SHLANG_SPIRV:
			return shader_cache_serialize_entry_metadata_spirv(meta, size, buf);

		default:
			log_error("Unknown shading language %u", meta->lang.lang);
			return 0;
	}
}

static bool shader_cache_entry_name(
	const CacheEntryMetadata *meta,
	size_t bufsize,
	char buf[bufsize]
) {
	uint8_t temp[bufsize];
	size_t meta_size = shader_cache_serialize_entry_metadata(meta, sizeof(temp), temp);

	if(UNLIKELY(meta_size == 0)) {
		return false;
	}

	const char *const prefixmap[] = {
		[SHLANG_GLSL - SHLANG_FIRST] = "glsl_",
		[SHLANG_SPIRV - SHLANG_FIRST] = "spirv_",
	};

	uint idx = meta->lang.lang - SHLANG_FIRST;
	assume(idx < sizeof(prefixmap));
	const char *prefix = prefixmap[idx];

	size_t prefix_size = strlen(prefix);
	assume(bufsize > prefix_size);
	memcpy(buf, prefix, prefix_size);
	buf += prefix_size;
	bufsize -= prefix_size;

	if(bufsize < meta_size * 2 + 1) {
		log_error("Filename is too long: buffer size is %zu, need at least %zu",
			bufsize + prefix_size, (size_t)(meta_size * 2 + 1 + prefix_size));
		return false;
	}

	hexdigest(temp, meta_size, buf, bufsize);
	return true;
}

bool spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	ShaderLangInfo target_lang = { 0 };
	target_lang.lang = SHLANG_SPIRV;
	target_lang.spirv.target = options->target;

	CacheEntryMetadata m = {
		.stage = in->stage,
		.lang = {
			.lang = SHLANG_SPIRV,
			.spirv.target = options->target,
		},
		.spirv = {
			.compile_options = options,
		},
	};

	if(!shader_cache_entry_name(&m, sizeof(name), name)) {
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

	CacheEntryMetadata m = {
		.stage = in->stage,
		.lang = *options->lang,
	};

	if(!shader_cache_entry_name(&m, sizeof(name), name)) {
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

	uint cflags = 0;
	// Preserve names of declarations.
	// This can be vital for shader interface matching.
	cflags |= SPIRV_CFLAG_DEBUG_INFO;

	if(target != SPIRV_TARGET_OPENGL_450) {
		cflags |= SPIRV_CFLAG_VULKAN_RELAXED;
	}

	result = spirv_compile(in, &spirv, &(SPIRVCompileOptions) {
		.target = target,
		.optimization_level = options->optimization_level,
		.filename = options->filename,
		.macros = macros,
		.flags = cflags,
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
