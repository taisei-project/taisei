/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_spirv_private.h"
#include "lang_dxbc.h"
#include "shaderlib.h"
#include "cache.h"

#include "log.h"
#include "memory/scratch.h"
#include "util.h"
#include "util/stringops.h"

typedef struct CacheEntryMetadata {
	ShaderLangInfo lang;
	ShaderStage stage;
	bool reflection;

	union {
		struct {
			const SPIRVCompileOptions *compile_options;
		} spirv;

		struct {
		} glsl;

		struct {
		} hlsl;

		struct {
			const DXBCCompileOptions *compile_options;
		} dxbc;
	};
} CacheEntryMetadata;

static size_t shader_cache_serialize_entry_metadata_glsl(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->reflection,
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
		meta->reflection,
		meta->lang.spirv.target,
		meta->spirv.compile_options->optimization_level,
		meta->spirv.compile_options->flags,
	};

	assume(sizeof(data) <= size);
	memcpy(buf, data, sizeof(data));
	return sizeof(data);
}

static size_t shader_cache_serialize_entry_metadata_hlsl(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->reflection,
		meta->lang.hlsl.shader_model,
	};

	assume(sizeof(data) <= size);
	memcpy(buf, data, sizeof(data));
	return sizeof(data);
}

static size_t shader_cache_serialize_entry_metadata_dxbc(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->reflection,
		meta->lang.dxbc.shader_model,
	};

	assume(sizeof(data) <= size);
	memcpy(buf, data, sizeof(data));
	return sizeof(data);
}

static size_t shader_cache_serialize_entry_metadata_msl(
	const CacheEntryMetadata *meta, size_t size, uint8_t buf[size]
) {
	uint8_t data[] = {
		meta->stage,
		meta->reflection,
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

		case SHLANG_HLSL:
			return shader_cache_serialize_entry_metadata_hlsl(meta, size, buf);

		case SHLANG_DXBC:
			return shader_cache_serialize_entry_metadata_dxbc(meta, size, buf);

		case SHLANG_MSL:
			return shader_cache_serialize_entry_metadata_msl(meta, size, buf);

		default:
			log_error("Unhandled shading language %s", shader_lang_name(meta->lang.lang));
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
		[SHLANG_HLSL - SHLANG_FIRST] = "hlsl_",
		[SHLANG_DXBC - SHLANG_FIRST] = "dxbc_",
		[SHLANG_MSL - SHLANG_FIRST] = "msl_",
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

bool spirv_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVCompileOptions *options,
	bool reflect
) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	ShaderLangInfo target_lang = {};
	target_lang.lang = SHLANG_SPIRV;
	target_lang.spirv.target = options->target;

	CacheEntryMetadata m = {
		.stage = in->stage,
		.reflection = reflect,
		.lang = {
			.lang = SHLANG_SPIRV,
			.spirv.target = options->target,
		},
		.spirv = {
			.compile_options = options,
		},
	};

	if(!shader_cache_entry_name(&m, sizeof(name), name)) {
		return _spirv_compile(in, out, arena, options);
	}

	if(!shader_cache_hash(in, options->macros, sizeof(hash), hash)) {
		return _spirv_compile(in, out, arena, options);
	}

	auto arena_snap = marena_snapshot(arena);

	if(shader_cache_get(hash, name, out, arena)) {
		if(
			!memcmp(&target_lang, &out->lang, sizeof(ShaderLangInfo)) &&
			out->stage == in->stage
		) {
			return true;
		}

		log_warn("Invalid cache entry ignored");
		marena_rollback(arena, &arena_snap);
	}

	if(_spirv_compile(in, out, arena, options)) {
		if(reflect) {
			if(!(out->reflection = _spirv_reflect(out, arena))) {
				marena_rollback(arena, &arena_snap);
				return false;
			}
		}

		shader_cache_set(hash, name, out);
		return true;
	}

	marena_rollback(arena, &arena_snap);
	return false;
}

bool spirv_decompile(const ShaderSource *in, ShaderSource *out, MemArena *arena, const SPIRVDecompileOptions *options) {
	char name[256], hash[SHADER_CACHE_HASH_BUFSIZE];

	CacheEntryMetadata m = {
		.stage = in->stage,
		.lang = *options->lang,
	};

	assume(m.lang.lang != SHLANG_SPIRV);

	if(!shader_cache_entry_name(&m, sizeof(name), name)) {
		return _spirv_decompile(in, out, arena, options);
	}

	if(!shader_cache_hash(in, NULL, sizeof(hash), hash)) {
		return _spirv_decompile(in, out, arena, options);
	}

	auto arena_snap = marena_snapshot(arena);
	bool result;

	if((result = shader_cache_get(hash, name, out, arena))) {
		if(
			!memcmp(options->lang, &out->lang, sizeof(ShaderLangInfo)) &&
			out->stage == in->stage
		) {
			return true;
		}

		log_warn("Invalid cache entry ignored");
		marena_rollback(arena, &arena_snap);
	}

	if((result = _spirv_decompile(in, out, arena, options))) {
		shader_cache_set(hash, name, out);
	}

	return result;
}

bool spirv_transpile(const ShaderSource *in, ShaderSource *out, MemArena *arena, const SPIRVTranspileOptions *options) {
	ShaderSource transient_src = {};
	bool result;

	ShaderSource _in = *in;
	in = &_in;

	SPIRVCompileOptions compile_opts = options->compile;
	SPIRVDecompileOptions decompile_opts = options->decompile;
	ShaderLangInfo decompile_lang = *decompile_opts.lang;
	decompile_opts.lang = &decompile_lang;

	bool compile_dxbc = false;

	if(decompile_lang.lang == SHLANG_DXBC) {
		decompile_lang.lang = SHLANG_HLSL;
		decompile_lang.hlsl.shader_model = options->decompile.lang->dxbc.shader_model;
		compile_dxbc = true;
	}

	bool reflect_on_compile = options->decompile.reflect && decompile_lang.lang == SHLANG_SPIRV;

	if(
		_in.lang.lang == SHLANG_GLSL &&
		!shader_lang_supports_uniform_locations(&_in.lang)
		// shader_lang_supports_uniform_locations(options->lang) &&
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

	char macro_buf[128];
	char *macro_buf_p = macro_buf;
	char *macro_buf_end = macro_buf + ARRAY_SIZE(macro_buf);
	ShaderMacro macros[32];
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
	ADD_MACRO_DYNAMIC("LANG_HLSL", "%d", SHLANG_HLSL);
	ADD_MACRO_DYNAMIC("LANG_DXBC", "%d", SHLANG_DXBC);
	ADD_MACRO_DYNAMIC("LANG_MSL", "%d", SHLANG_MSL);
	ADD_MACRO_DYNAMIC("TRANSPILE_TARGET_LANG", "%d", decompile_lang.lang);

	switch(decompile_lang.lang) {
		case SHLANG_GLSL:
			ADD_MACRO_BOOL("TRANSPILE_TARGET_GLSL_ES", decompile_lang.glsl.version.profile == GLSL_PROFILE_ES);
			ADD_MACRO_DYNAMIC("TRANSPILE_TARGET_GLSL_VERSION", "%d", decompile_lang.glsl.version.version);
			break;

		case SHLANG_HLSL:
			ADD_MACRO_DYNAMIC("TRANSPILE_TARGET_HLSL_SHADER_MODEL", "%d", decompile_lang.hlsl.shader_model);
			ADD_MACRO_BOOL("TRANSPILE_TARGET_HLSL_FOR_DXBC", compile_dxbc);
			break;

		case SHLANG_SPIRV:
		case SHLANG_MSL:
			break;

		default: UNREACHABLE;
	}

	ADD_MACRO(NULL);

	compile_opts.macros = macros;

	if(decompile_lang.lang == SHLANG_SPIRV) {
		compile_opts.target = options->decompile.lang->spirv.target;
	}

	// Preserve names of declarations.
	// This can be vital for shader interface matching.
	compile_opts.flags |= SPIRV_CFLAG_DEBUG_INFO;

	if(compile_opts.target != SPIRV_TARGET_OPENGL_450) {
		compile_opts.flags |= SPIRV_CFLAG_VULKAN_RELAXED;
	}

	result = spirv_compile(in, &transient_src, arena, &compile_opts, reflect_on_compile);

	if(!result) {
		return false;
	}

	if(decompile_lang.lang == SHLANG_SPIRV) {
		assert(!decompile_opts.reflect || transient_src.reflection);
		*out = transient_src;

#ifdef DEBUG
		auto scratch = acquire_scratch_arena();
		ShaderSource decomp = {};
		spirv_decompile(out, &decomp, scratch, &(SPIRVDecompileOptions) {
			.lang = &(ShaderLangInfo) {
				.lang = SHLANG_GLSL,
				.glsl.version.version = 460,
				.glsl.version.profile = GLSL_PROFILE_CORE,
			},
			.glsl.vulkan_semantics = (compile_opts.target != SPIRV_TARGET_OPENGL_450),
		});

		log_debug("Decompiled source:\n%s\n\n ", decomp.content);
		release_scratch_arena(scratch);
#endif

		return result;
	}

	result = spirv_decompile(&transient_src, out, arena, &decompile_opts);

	if(!result) {
		return false;
	}

	log_debug("%s: translated code:\n%s", compile_opts.filename, out->content);

	if(compile_dxbc) {
		assert(out->lang.lang == SHLANG_HLSL);
		result = dxbc_compile(out, &transient_src, arena, &(DXBCCompileOptions) {
			.entrypoint = "main",
		});

		if(result) {
			transient_src.reflection = out->reflection;
			*out = transient_src;
			assert(out->lang.dxbc.shader_model == options->decompile.lang->dxbc.shader_model);
		}
	}

	return result;
}
