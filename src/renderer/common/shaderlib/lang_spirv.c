/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shaderlib.h"
#include "lang_spirv_private.h"

#include "util.h"

#include <shaderc/shaderc.h>
#include <spirv_cross_c.h>

static shaderc_compiler_t spirv_compiler;

static inline shaderc_optimization_level resolve_opt_level(SPIRVOptimizationLevel lvl) {
	switch(lvl) {
		case SPIRV_OPTIMIZE_NONE:        return shaderc_optimization_level_zero;
		case SPIRV_OPTIMIZE_SIZE:        return shaderc_optimization_level_size;
		case SPIRV_OPTIMIZE_PERFORMANCE: return shaderc_optimization_level_performance;
		default: UNREACHABLE;
	}
}

static inline shaderc_profile resolve_glsl_profile(const GLSLVersion *v) {
	switch(v->profile) {
		case GLSL_PROFILE_COMPATIBILITY: return shaderc_profile_compatibility;
		case GLSL_PROFILE_CORE:          return shaderc_profile_core;
		case GLSL_PROFILE_ES:            return shaderc_profile_es;
		case GLSL_PROFILE_NONE:          return shaderc_profile_none;
		default: UNREACHABLE;
	}
}

static inline shaderc_target_env resolve_env(SPIRVTarget target, uint32_t *out_version) {
	switch(target) {
		case SPIRV_TARGET_OPENGL_450:
			*out_version = shaderc_env_version_opengl_4_5;
			return shaderc_target_env_opengl;

		case SPIRV_TARGET_VULKAN_10:
			*out_version = shaderc_env_version_vulkan_1_0;
			return shaderc_target_env_vulkan;

		case SPIRV_TARGET_VULKAN_11:
			*out_version = shaderc_env_version_vulkan_1_1;
			return shaderc_target_env_vulkan;

		default: UNREACHABLE;
	}
}

static inline shaderc_shader_kind resolve_kind(ShaderStage stage) {
	switch(stage) {
		case SHADER_STAGE_FRAGMENT: return shaderc_fragment_shader;
		case SHADER_STAGE_VERTEX:   return shaderc_vertex_shader;
		default: UNREACHABLE;
	}
}

void spirv_init_compiler(void) {
	if(spirv_compiler == NULL) {
		spirv_compiler = shaderc_compiler_initialize();
		if(spirv_compiler == NULL) {
			log_error("Failed to initialize the compiler");
		}
	}
}

void spirv_shutdown_compiler(void) {
	if(spirv_compiler != NULL) {
		shaderc_compiler_release(spirv_compiler);
		spirv_compiler = NULL;
	}
}

bool _spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) {
	if(in->lang.lang != SHLANG_GLSL) {
		log_error("Unsupported source language");
		return false;
	}

	if(spirv_compiler == NULL) {
		log_error("Compiler is not initialized");
		return false;
	}

	shaderc_compile_options_t opts = shaderc_compile_options_initialize();

	if(opts == NULL) {
		log_error("Failed to initialize compiler options");
		return false;
	}

	shaderc_compile_options_set_source_language(opts, shaderc_source_language_glsl);
	shaderc_compile_options_set_optimization_level(opts, resolve_opt_level(options->optimization_level));
	shaderc_compile_options_set_forced_version_profile(opts, in->lang.glsl.version.version, resolve_glsl_profile(&in->lang.glsl.version));
	shaderc_compile_options_set_auto_map_locations(opts, true);

	uint32_t env_version;
	shaderc_target_env env = resolve_env(options->target, &env_version);
	shaderc_compile_options_set_target_env(opts, env, env_version);

	if(options->debug_info) {
		shaderc_compile_options_set_generate_debug_info(opts);
	}

	if(options->macros) {
		for(ShaderMacro *m = options->macros; m->name; ++m) {
			shaderc_compile_options_add_macro_definition(opts, m->name, strlen(m->name), m->value, strlen(m->value));
		}
	}

	const char *filename = options->filename ? options->filename : "<main>";

	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		spirv_compiler,
		in->content,
		in->content_size - 1,
		resolve_kind(in->stage),
		filename,
		"main",
		opts
	);

	shaderc_compile_options_release(opts);

	if(result == NULL) {
		log_error("Failed to allocate compilation result");
		return false;
	}

	shaderc_compilation_status status = shaderc_result_get_compilation_status(result);

	switch(status) {
		case shaderc_compilation_status_compilation_error:
			log_error("Compilation failed: errors in the shader");
			break;

		case shaderc_compilation_status_internal_error:
			log_error("Compilation failed: internal compiler error");
			break;

		case shaderc_compilation_status_invalid_assembly:
			log_error("Compilation failed: invalid assembly");
			break;

		case shaderc_compilation_status_invalid_stage:
			log_error("Compilation failed: invalid shader stage");
			break;

		case shaderc_compilation_status_null_result_object:
			UNREACHABLE;

		case shaderc_compilation_status_success:
			break;

		default:
			log_error("Compilation failed: unknown error");
			break;
	}

	const char *err_str = shaderc_result_get_error_message(result);

	if(err_str != NULL && *err_str) {
		size_t num_warnings = shaderc_result_get_num_warnings(result);
		size_t num_errors = shaderc_result_get_num_errors(result);
		LogLevel lvl = status == shaderc_compilation_status_success ? LOG_WARN : LOG_ERROR;
		log_custom(lvl, "%s: %zu warnings, %zu errors:\n\n%s", filename, num_warnings, num_errors, err_str);
	}

	if(status != shaderc_compilation_status_success) {
		shaderc_result_release(result);
		return false;
	}

	size_t data_len = shaderc_result_get_length(result);
	const char *data = shaderc_result_get_bytes(result);

	memset(out, 0, sizeof(*out));
	out->stage = in->stage;
	out->lang.lang = SHLANG_SPIRV;
	out->lang.spirv.target = options->target;
	out->content_size = data_len + 1;
	out->content = mem_alloc(out->content_size);
	memcpy(out->content, data, data_len);
	shaderc_result_release(result);

	return true;
}

#define SPVCCALL(c) do if((spvc_return_code = (c)) != SPVC_SUCCESS) { goto spvc_error; } while(0)

static spvc_result write_glsl_attribs(spvc_compiler compiler, ShaderSource *out) {
	spvc_result spvc_return_code = SPVC_SUCCESS;

	spvc_resources resources;
	const spvc_reflected_resource *inputs;
	size_t num_inputs;

	SPVCCALL(spvc_compiler_create_shader_resources(compiler, &resources));
	SPVCCALL(spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &inputs, &num_inputs));

	log_debug("%zu stage inputs", num_inputs);

	// caller is expected to clean this up in case of error
	auto attrs = ALLOC_ARRAY(num_inputs, GLSLAttribute);
	out->meta.glsl.attributes = attrs;
	out->meta.glsl.num_attributes = num_inputs;

	for(uint i = 0; i < num_inputs; ++i) {
		const spvc_reflected_resource *res = inputs + i;
		uint location = spvc_compiler_get_decoration(compiler, res->id, SpvDecorationLocation);

		log_debug("[%i] %s\t\tid=%i\t\tbase_type_id=%i\t\ttype_id=%i\t\tlocation=%i", i, res->name, res->id, res->base_type_id, res->type_id, location);

		attrs[i].name = strdup(res->name);
		attrs[i].location = location;
	}

spvc_error:
	return spvc_return_code;
}

bool _spirv_decompile(const ShaderSource *in, ShaderSource *out, const SPIRVDecompileOptions *options) {
	if(in->lang.lang != SHLANG_SPIRV) {
		log_error("Source is not a SPIR-V binary");
		return false;
	}

	if(options->lang->lang != SHLANG_GLSL) {
		log_error("Target language is not supported");
		return false;
	}

	size_t spirv_size = (in->content_size - 1) / sizeof(uint32_t);
	const uint32_t *spirv = (uint32_t*)(void*)in->content;

	spvc_result spvc_return_code = SPVC_SUCCESS;
	spvc_context context = NULL;
	spvc_parsed_ir ir = NULL;
	spvc_compiler compiler = NULL;
	spvc_compiler_options spvc_options = NULL;
	const char *code = NULL;
	spvc_context_create(&context);

	if(context == NULL) {
		log_error("Failed to initialize SPIRV-Cross");
		return false;
	}

	memset(out, 0, sizeof(*out));

	SPVCCALL(spvc_context_parse_spirv(context, spirv, spirv_size, &ir));
	SPVCCALL(spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler));

	if(in->stage == SHADER_STAGE_VERTEX && !shader_lang_supports_attribute_locations(options->lang)) {
		SPVCCALL(write_glsl_attribs(compiler, out));
	}

	if(!glsl_version_supports_instanced_rendering(options->lang->glsl.version)) {
		SPVCCALL(spvc_compiler_add_header_line(compiler,
			"#ifdef GL_ARB_draw_instanced\n"
			"#extension GL_ARB_draw_instanced : enable\n"
			"#endif\n"

			"#ifdef GL_EXT_frag_depth\n"
			"#extension GL_EXT_frag_depth : enable\n"
			"#define gl_FragDepth gl_FragDepthEXT\n"
			"#endif\n"
		));
	}

	SPVCCALL(spvc_compiler_create_compiler_options(compiler, &spvc_options));
	SPVCCALL(spvc_compiler_options_set_uint(spvc_options, SPVC_COMPILER_OPTION_GLSL_VERSION, options->lang->glsl.version.version));
	SPVCCALL(spvc_compiler_options_set_bool(spvc_options, SPVC_COMPILER_OPTION_GLSL_ES, options->lang->glsl.version.profile == GLSL_PROFILE_ES));
	SPVCCALL(spvc_compiler_options_set_bool(spvc_options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, false));
	SPVCCALL(spvc_compiler_install_compiler_options(compiler, spvc_options));
	SPVCCALL(spvc_compiler_compile(compiler, &code));

	assume(code != NULL);

	out->content = strdup(code);
	out->content_size = strlen(code) + 1;
	out->stage = in->stage;
	out->lang = *options->lang;

	spvc_context_destroy(context);
	return true;

spvc_error:
	log_error("SPIRV-Cross error: %s", spvc_context_get_last_error_string(context));
	glsl_free_source(out);
	spvc_context_destroy(context);
	return false;
}
