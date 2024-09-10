/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_spirv_private.h"
#include "shaderlib.h"
#include "reflect.h"

#include "log.h"
#include "util.h"

#include <shaderc/shaderc.h>
#include <spirv_cross_c.h>

typedef struct SPIRVReflectContext {
	MemArena *arena;
	ShaderReflection *reflection;
	struct {
		spvc_context ctx;
		spvc_compiler compiler;
		spvc_resources res;
	} spvc;
} SPIRVReflectContext;

static spvc_result _spirv_reflect_all(SPIRVReflectContext *rctx);

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

bool _spirv_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVCompileOptions *options
) {
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
	// shaderc_compile_options_set_auto_map_locations(opts, true);

	uint32_t env_version;
	shaderc_target_env env = resolve_env(options->target, &env_version);
	shaderc_compile_options_set_target_env(opts, env, env_version);

	if(options->flags & SPIRV_CFLAG_DEBUG_INFO) {
		shaderc_compile_options_set_generate_debug_info(opts);
	}

	if(env == shaderc_target_env_vulkan && (options->flags & SPIRV_CFLAG_VULKAN_RELAXED)) {
		// FIXME: these are hardcoded for SDLGPU requirements.
		// We should expose these in SPIRVCompileOptions and move shader translation logic into the backends.

		shaderc_compile_options_set_vulkan_rules_relaxed(opts, true);
		shaderc_compile_options_set_auto_bind_uniforms(opts, true);

		if(in->stage == SHADER_STAGE_VERTEX) {
			shaderc_compile_options_set_default_uniform_block_set_and_binding(opts, 1, 0);
		} else {
			shaderc_compile_options_set_binding_descriptor_set(opts, 2);
			shaderc_compile_options_set_default_uniform_block_set_and_binding(opts, 3, 0);
		}

		// Required to eliminate unused samplers (FIXME: filter these out during reflection if possible?)
		shaderc_compile_options_set_optimization_level(opts, SPIRV_OPTIMIZE_PERFORMANCE);
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
	char *data_copy = marena_memdup(arena, data, data_len);
	shaderc_result_release(result);

	*out = (ShaderSource) {
		.stage = in->stage,
		.lang.lang = SHLANG_SPIRV,
		.lang.spirv.target = options->target,
		.content_size = data_len,
		.content = data_copy,
		.entrypoint = marena_strdup(arena, in->entrypoint),
	};

	return true;
}

#define SPVCCALL(c) do if((spvc_return_code = (c)) != SPVC_SUCCESS) { goto spvc_error; } while(0)

bool _spirv_decompile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVDecompileOptions *options
) {
	if(UNLIKELY(in->lang.lang != SHLANG_SPIRV)) {
		log_error("Source is not a SPIR-V binary");
		return false;
	}

	spvc_backend backend;

	switch(options->lang->lang) {
		case SHLANG_GLSL:      backend = SPVC_BACKEND_GLSL; break;
		case SHLANG_HLSL:      backend = SPVC_BACKEND_HLSL; break;
		case SHLANG_MSL:       backend = SPVC_BACKEND_MSL;  break;
		default:
			log_error("Can't decompile SPIR-V into %s", shader_lang_name(options->lang->lang));
			return false;
	}

	size_t spirv_size = in->content_size / sizeof(uint32_t);
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

	auto arena_snap = marena_snapshot(arena);

	SPVCCALL(spvc_context_parse_spirv(context, spirv, spirv_size, &ir));
	SPVCCALL(spvc_context_create_compiler(context, backend, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler));

	SPVCCALL(spvc_compiler_create_compiler_options(compiler, &spvc_options));

	if(backend == SPVC_BACKEND_GLSL) {
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

		SPVCCALL(spvc_compiler_options_set_uint(
			spvc_options, SPVC_COMPILER_OPTION_GLSL_VERSION, options->lang->glsl.version.version));
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_GLSL_ES, options->lang->glsl.version.profile == GLSL_PROFILE_ES));
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, false));
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, options->glsl.vulkan_semantics));
	} else if(backend == SPVC_BACKEND_HLSL) {
		SPVCCALL(spvc_compiler_options_set_uint(
			spvc_options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, options->lang->hlsl.shader_model));
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_HLSL_NONWRITABLE_UAV_TEXTURE_AS_SRV, true));
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_HLSL_FLATTEN_MATRIX_VERTEX_INPUT_SEMANTICS, true));
	} else if(backend == SPVC_BACKEND_MSL) {
		SPVCCALL(spvc_compiler_options_set_bool(
			spvc_options, SPVC_COMPILER_OPTION_MSL_ENABLE_DECORATION_BINDING, true));
	} else {
		UNREACHABLE;
	}

	SPVCCALL(spvc_compiler_install_compiler_options(compiler, spvc_options));
	SPVCCALL(spvc_compiler_compile(compiler, &code));
	assume(code != NULL);

	size_t code_size = strlen(code) + 1;

	const spvc_entry_point *entry_points;
	size_t num_entry_points;

	SPVCCALL(spvc_compiler_get_entry_points(compiler, &entry_points, &num_entry_points));

	if(num_entry_points != 1) {
		log_error("Shader has %u entry points, 1 expected", (uint)num_entry_points);
		goto error;
	}

	const char *entrypoint = spvc_compiler_get_cleansed_entry_point_name(
		compiler, entry_points[0].name, entry_points[0].execution_model);

	*out = (ShaderSource) {
		.content = marena_memdup(arena, code, code_size),
		.content_size = code_size,
		.entrypoint = marena_strdup(arena, entrypoint),
		.stage = in->stage,
		.lang = *options->lang,
	};

	if(options->reflect) {
		SPIRVReflectContext rctx = {
			.arena = arena,
			.reflection = ARENA_ALLOC(arena, typeof(*rctx.reflection), {}),
			.spvc = {
				.compiler = compiler,
				.ctx = context,
			},
		};

		SPVCCALL(spvc_compiler_create_shader_resources(rctx.spvc.compiler, &rctx.spvc.res));
		SPVCCALL(_spirv_reflect_all(&rctx));

		out->reflection = rctx.reflection;
	}

	spvc_context_destroy(context);
	return true;

spvc_error:
	log_error("SPIRV-Cross error: %s", spvc_context_get_last_error_string(context));
error:
	spvc_context_destroy(context);
	marena_rollback(arena, &arena_snap);
	return false;
}

static uint spvc_compiler_get_member_decoration_fallback(
	spvc_compiler comp, spvc_type_id id, uint member_idx, SpvDecoration dec, uint fallback
) {
	if(!spvc_compiler_has_member_decoration(comp, id, member_idx, dec)) {
		return fallback;
	}

	return spvc_compiler_get_member_decoration(comp, id, member_idx, dec);
}

static uint spvc_compiler_get_decoration_fallback(
	spvc_compiler comp, spvc_type_id id, SpvDecoration dec, uint fallback
) {
	if(!spvc_compiler_has_decoration(comp, id, dec)) {
		return fallback;
	}

	return spvc_compiler_get_decoration(comp, id, dec);
}

static uint get_collapsed_array_dimensions(spvc_type type) {
	uint dims = spvc_type_get_num_array_dimensions(type);

	if(dims == 0) {
		return 0;
	}

	uint s = 1;
	for(uint d = 0; d < dims; ++d) {
		SpvId dim = spvc_type_get_array_dimension(type, d);
		assert(spvc_type_array_dimension_is_literal(type, d));
		s *= dim;
	}

	return s;
}

static bool _spirv_reflect_ubos(SPIRVReflectContext *rctx) {
	spvc_result spvc_return_code = SPVC_SUCCESS;
	auto reflection = rctx->reflection;
	const spvc_reflected_resource *res_list;
	size_t res_size;

	SPVCCALL(spvc_resources_get_resource_list_for_type(
		rctx->spvc.res, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &res_list, &res_size));

	reflection->num_uniform_buffers = res_size;
	reflection->uniform_buffers = ARENA_ALLOC_ARRAY(rctx->arena, res_size, typeof(*reflection->uniform_buffers));

	for(size_t i = 0; i < res_size; ++i) {
		auto res = &res_list[i];

		auto buf_typeid = res->base_type_id;
		auto buf_type = spvc_compiler_get_type_handle(rctx->spvc.compiler, buf_typeid);

		size_t buf_size = 0;
		spvc_compiler_get_declared_struct_size(rctx->spvc.compiler, buf_type, &buf_size);

		uint num_members = spvc_type_get_num_member_types(buf_type);

		auto block = &reflection->uniform_buffers[i];
		*block = (ShaderBlock) {
			.name = marena_strdup(rctx->arena, res->name),
			.set = spvc_compiler_get_decoration_fallback(rctx->spvc.compiler, res->id, SpvDecorationDescriptorSet, 0),
			.binding = spvc_compiler_get_decoration_fallback(rctx->spvc.compiler, res->id, SpvDecorationBinding, 0),
			.num_fields = num_members,
			.size = buf_size,
			.fields = ARENA_ALLOC_ARRAY(rctx->arena, num_members, typeof(*reflection->uniform_buffers[i].fields)),
		};

		for(uint m = 0; m < num_members; ++m) {
			spvc_type_id mtypeid = spvc_type_get_member_type(buf_type, m);
			spvc_type mtype = spvc_compiler_get_type_handle(rctx->spvc.compiler, mtypeid);

			spvc_basetype mbasetype = spvc_type_get_basetype(mtype);
			assert(spvc_type_get_bit_width(mtype) == 8 * shader_basetype_size(mbasetype));

			uint mtype_vecsize = spvc_type_get_vector_size(mtype);

			block->fields[m] = (ShaderStructField) {
				.name = marena_strdup(rctx->arena, spvc_compiler_get_member_name(rctx->spvc.compiler, buf_typeid, m)),
				.offset = spvc_compiler_get_member_decoration_fallback(
					rctx->spvc.compiler, buf_typeid, m, SpvDecorationOffset, 0),
				.type = {
					.base_type = spvc_type_get_basetype(mtype),
					.array_size = get_collapsed_array_dimensions(mtype),
					.array_stride = spvc_compiler_get_decoration_fallback(
						rctx->spvc.compiler, mtypeid, SpvDecorationArrayStride, 0),
					.matrix_stride = ({
						uint stride = spvc_compiler_get_member_decoration_fallback(
							rctx->spvc.compiler, buf_typeid, m, SpvDecorationMatrixStride, 0);

						if(stride == 1) {
							// NOTE: Assumes std140 layout
							stride = 16 * ((shader_basetype_size(mbasetype) * mtype_vecsize - 1) / 16 + 1);
							log_warn("SPIRV-Cross bug: matrix stride query returned 1; assuming %d. "
							         "Please update SPIRV-Cross", stride);
						}

						stride;
					}),
					.matrix_columns = spvc_type_get_columns(mtype),
					.vector_size = mtype_vecsize,
				},
			};
		}
	}

spvc_error:
	return spvc_return_code;
}

static ShaderSamplerDimension sampler_dim_from_spv_dim(SpvDim dim) {
	switch(dim) {
		case SpvDim1D:			return SHADER_SAMPLER_DIM_1D;
		case SpvDim2D:			return SHADER_SAMPLER_DIM_2D;
		case SpvDim3D:			return SHADER_SAMPLER_DIM_3D;
		case SpvDimCube:		return SHADER_SAMPLER_DIM_CUBE;
		case SpvDimBuffer:		return SHADER_SAMPLER_DIM_BUFFER;
		default:				return SHADER_SAMPLER_DIM_UNKNOWN;
	}
}

static ShaderSamplerType sampler_type_from_spvc_type(spvc_type type) {
	ShaderSamplerTypeFlags flags = 0;
	flags |= spvc_type_get_image_is_depth(type)     ? SHADER_SAMPLER_DEPTH : 0;
	flags |= spvc_type_get_image_arrayed(type)      ? SHADER_SAMPLER_ARRAYED : 0;
	flags |= spvc_type_get_image_multisampled(type) ? SHADER_SAMPLER_MULTISAMPLED : 0;

	return (ShaderSamplerType) {
		.dim = sampler_dim_from_spv_dim(spvc_type_get_image_dimension(type)),
		.flags = flags,
	};
}

static bool _spirv_reflect_samplers(SPIRVReflectContext *rctx) {
	spvc_result spvc_return_code = SPVC_SUCCESS;
	auto reflection = rctx->reflection;
	const spvc_reflected_resource *res_list;
	size_t res_size;

	SPVCCALL(spvc_resources_get_resource_list_for_type(
		rctx->spvc.res, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, &res_list, &res_size));

	reflection->num_samplers = res_size;
	reflection->samplers = ARENA_ALLOC_ARRAY(rctx->arena, res_size, typeof(*reflection->samplers));

	for(uint i = 0; i < res_size; ++i) {
		auto res = &res_list[i];
		auto type = spvc_compiler_get_type_handle(rctx->spvc.compiler, res->type_id);

		reflection->samplers[i] = (ShaderSampler) {
			.name = marena_strdup(rctx->arena, res->name),
			.set = spvc_compiler_get_decoration_fallback(rctx->spvc.compiler, res->id, SpvDecorationDescriptorSet, 0),
			.binding = spvc_compiler_get_decoration_fallback(rctx->spvc.compiler, res->id, SpvDecorationBinding, 0),
			.array_size = get_collapsed_array_dimensions(type),
			.type = sampler_type_from_spvc_type(type),
		};
	}

spvc_error:
	return spvc_return_code;
}

static bool _spirv_reflect_inputs(SPIRVReflectContext *rctx) {
	spvc_result spvc_return_code = SPVC_SUCCESS;
	auto reflection = rctx->reflection;
	const spvc_reflected_resource *res_list;
	size_t res_size;

	SPVCCALL(spvc_resources_get_resource_list_for_type(
		rctx->spvc.res, SPVC_RESOURCE_TYPE_STAGE_INPUT, &res_list, &res_size));

	reflection->num_inputs = res_size;
	reflection->inputs = ARENA_ALLOC_ARRAY(rctx->arena, res_size, typeof(*reflection->inputs));

	reflection->used_input_locations_map = 0;

	for(uint i = 0; i < res_size; ++i) {
		auto res = &res_list[i];
		auto type = spvc_compiler_get_type_handle(rctx->spvc.compiler, res->type_id);

		uint loc = spvc_compiler_get_decoration_fallback(rctx->spvc.compiler, res->id, SpvDecorationLocation, 0);
		uint uses_locs = spvc_type_get_columns(type) ?: 1;  // FIXME not sure if this is always correct

		reflection->used_input_locations_map |= ((1ull << (loc + uses_locs)) - 1ull) & ~((1ull << loc) - 1ull);

		reflection->inputs[i] = (ShaderInput) {
			.name = marena_strdup(rctx->arena, res->name),
			.location = loc,
			.num_locations_consumed = uses_locs,
		};
	}

spvc_error:
	return spvc_return_code;
}

ShaderReflection *_spirv_reflect(const ShaderSource *src, MemArena *arena) {
	spvc_result spvc_return_code = SPVC_SUCCESS;

	SPIRVReflectContext rctx = {
		.arena = arena,
	};

	spvc_parsed_ir ir = NULL;

	spvc_context_create(&rctx.spvc.ctx);

	if(rctx.spvc.ctx == NULL) {
		log_error("Failed to initialize SPIRV-Cross");
		return NULL;
	}

	rctx.reflection = ARENA_ALLOC(arena, typeof(*rctx.reflection), {});

	size_t spirv_size = src->content_size / sizeof(uint32_t);
	const uint32_t *spirv = (uint32_t*)(void*)src->content;

	SPVCCALL(spvc_context_parse_spirv(rctx.spvc.ctx, spirv, spirv_size, &ir));
	SPVCCALL(spvc_context_create_compiler(
		rctx.spvc.ctx, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &rctx.spvc.compiler));
	SPVCCALL(spvc_compiler_create_shader_resources(rctx.spvc.compiler, &rctx.spvc.res));

	SPVCCALL(_spirv_reflect_all(&rctx));

	spvc_context_destroy(rctx.spvc.ctx);
	return rctx.reflection;

spvc_error:
	log_error("SPIRV-Cross error: %s", spvc_context_get_last_error_string(rctx.spvc.ctx));
	spvc_context_destroy(rctx.spvc.ctx);
	return NULL;
}

static spvc_result _spirv_reflect_all(SPIRVReflectContext *rctx) {
	spvc_result spvc_return_code = SPVC_SUCCESS;

	SPVCCALL(_spirv_reflect_ubos(rctx));
	SPVCCALL(_spirv_reflect_samplers(rctx));
	SPVCCALL(_spirv_reflect_inputs(rctx));

spvc_error:
	return spvc_return_code;
}
