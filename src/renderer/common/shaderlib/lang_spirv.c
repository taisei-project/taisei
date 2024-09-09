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
#include "memory/scratch.h"
#include "util.h"

#include <spirv_cross_c.h>

DIAGNOSTIC(push)
DIAGNOSTIC(ignored "-Wstrict-prototypes")
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
DIAGNOSTIC(pop)

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
static bool compiler_initialized;

static inline glslang_profile_t resolve_glsl_profile(const GLSLVersion *v) {
	switch(v->profile) {
		case GLSL_PROFILE_COMPATIBILITY: return GLSLANG_COMPATIBILITY_PROFILE;
		case GLSL_PROFILE_CORE:          return GLSLANG_CORE_PROFILE;
		case GLSL_PROFILE_ES:            return GLSLANG_ES_PROFILE;
		case GLSL_PROFILE_NONE:          return GLSLANG_NO_PROFILE;
		default: UNREACHABLE;
	}
}

static inline glslang_client_t resolve_client(SPIRVTarget target) {
	switch(target) {
		case SPIRV_TARGET_OPENGL_450: return GLSLANG_CLIENT_OPENGL;
		case SPIRV_TARGET_VULKAN_10:  return GLSLANG_CLIENT_VULKAN;
		case SPIRV_TARGET_VULKAN_11:  return GLSLANG_CLIENT_VULKAN;
		default: UNREACHABLE;
	}
}

static inline glslang_target_client_version_t resolve_client_version(SPIRVTarget target) {
	switch(target) {
		case SPIRV_TARGET_OPENGL_450: return GLSLANG_TARGET_OPENGL_450;
		case SPIRV_TARGET_VULKAN_10:  return GLSLANG_TARGET_VULKAN_1_0;
		case SPIRV_TARGET_VULKAN_11:  return GLSLANG_TARGET_VULKAN_1_1;
		default: UNREACHABLE;
	}
}

static inline glslang_stage_t resolve_stage(ShaderStage stage) {
	switch(stage) {
		case SHADER_STAGE_FRAGMENT: return GLSLANG_STAGE_FRAGMENT;
		case SHADER_STAGE_VERTEX:   return GLSLANG_STAGE_VERTEX;
		default: UNREACHABLE;
	}
}

void spirv_init_compiler(void) {
	if(!compiler_initialized) {
		glslang_initialize_process();
		compiler_initialized = true;
	}
}

void spirv_shutdown_compiler(void) {
	if(compiler_initialized) {
		glslang_finalize_process();
		compiler_initialized = false;
	}
}

static void print_shader_log_message(
	MemArena *scratch,
	const char *filename,
	const char *prefix,
	LogLevel level,
	const char *msg
) {
	size_t msglen;

	if(!msg || !(msglen = strlen(msg))) {
		return;
	}

	auto snap = marena_snapshot(scratch);
	char *buf = marena_memdup(scratch, msg, msglen + 1);

	while(buf[msglen - 1] == '\n') {
		buf[--msglen] = 0;

		if(!msglen) {
			marena_rollback(scratch, &snap);
			return;
		}
	}

	log_custom(level, "%s: %s:\n%s", filename, prefix, buf);
	marena_rollback(scratch, &snap);
}

static void print_shader_log(glslang_shader_t *shader, LogLevel level, const SPIRVCompileOptions *options) {
	auto scratch = acquire_scratch_arena();

	print_shader_log_message(scratch, options->filename, "glslang shader log", level,
		glslang_shader_get_info_log(shader));
	print_shader_log_message(scratch, options->filename, "glslang shader debug log", level,
		glslang_shader_get_info_debug_log(shader));

	release_scratch_arena(scratch);
}

static void print_program_log(glslang_program_t *prog, LogLevel level, const SPIRVCompileOptions *options) {
	auto scratch = acquire_scratch_arena();

	print_shader_log_message(scratch, options->filename, "glslang program log", level,
		glslang_program_get_info_log(prog));
	print_shader_log_message(scratch, options->filename, "glslang program debug log", level,
		glslang_program_get_info_debug_log(prog));

	release_scratch_arena(scratch);
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

	if(!compiler_initialized) {
		log_error("Compiler is not initialized");
		return false;
	}

	auto input = (glslang_input_t) {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = resolve_stage(in->stage),
		.client = resolve_client(options->target),
		.client_version = resolve_client_version(options->target),
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_0,
		.default_version = in->lang.glsl.version.version,
		.default_profile = resolve_glsl_profile(&in->lang.glsl.version),
		.force_default_version_and_profile = true,
		.messages = GLSLANG_MSG_ENHANCED,
		.resource = glslang_default_resource(),
		.code = in->content,
	};

	glslang_messages_t spirv_messages = GLSLANG_MSG_ENHANCED | GLSLANG_MSG_SPV_RULES_BIT;

	if(options->flags & SPIRV_CFLAG_DEBUG_INFO) {
		spirv_messages |= GLSLANG_MSG_DEBUG_INFO_BIT;
	}

	if(input.client == GLSLANG_CLIENT_VULKAN) {
		spirv_messages |= GLSLANG_MSG_VULKAN_RULES_BIT;
	}

	glslang_shader_t *shader = glslang_shader_create(&input);
	glslang_shader_options_t shader_opts = GLSLANG_SHADER_DEFAULT_BIT;

	if(input.client == GLSLANG_CLIENT_VULKAN && (options->flags & SPIRV_CFLAG_VULKAN_RELAXED)) {
		shader_opts |= GLSLANG_SHADER_VULKAN_RULES_RELAXED | GLSLANG_SHADER_AUTO_MAP_BINDINGS;

		// FIXME: these are hardcoded for SDLGPU requirements.
		if(in->stage == SHADER_STAGE_VERTEX) {
			glslang_shader_set_default_uniform_block_set_and_binding(shader, 1, 0);
		} else {
			glslang_shader_set_resource_set_binding(shader, (const char*[]) { "2" }, 1);
			glslang_shader_set_default_uniform_block_set_and_binding(shader, 3, 0);
		}
	}

	StringBuffer macros_buf = {};

	if(options->macros) {
		for(ShaderMacro *m = options->macros; m->name; ++m) {
			strbuf_printf(&macros_buf, "#define %s %s\n", m->name, m->value);
		}

		glslang_shader_set_preamble(shader, macros_buf.start);
	}

	glslang_shader_set_options(shader, shader_opts);
	glslang_shader_set_glsl_version(shader, in->lang.glsl.version.version);

	bool ok = glslang_shader_preprocess(shader, &input);

	if(!ok) {
		print_shader_log(shader, LOG_ERROR, options);
		log_error("%s: glslang_shader_preprocess() failed", options->filename);
		glslang_shader_delete(shader);
		return false;
	}

	ok = glslang_shader_parse(shader, &input);
	strbuf_free(&macros_buf);

	if(!ok) {
		print_shader_log(shader, LOG_ERROR, options);
		log_error("%s: glslang_shader_parse() failed", options->filename);
		glslang_shader_delete(shader);
		return false;
	}

	print_shader_log(shader, LOG_WARN, options);

	glslang_program_t *prog = glslang_program_create();
	glslang_program_add_shader(prog, shader);
	ok = glslang_program_link(prog, spirv_messages);

	if(!ok) {
		print_program_log(prog, LOG_ERROR, options);
		log_error("%s: glslang_program_link() failed", options->filename);
		glslang_program_delete(prog);
		glslang_shader_delete(shader);
		return false;
	}

	ok = glslang_program_map_io(prog);

	if(!ok) {
		print_program_log(prog, LOG_ERROR, options);
		log_error("%s: glslang_program_map_io() failed", options->filename);
		glslang_program_delete(prog);
		glslang_shader_delete(shader);
		return false;
	}

	print_program_log(prog, LOG_WARN, options);

	glslang_program_SPIRV_generate_with_options(prog, input.stage, &(glslang_spv_options_t) {
		.generate_debug_info = (options->flags & SPIRV_CFLAG_DEBUG_INFO),
		.optimize_size = options->optimization_level != SPIRV_OPTIMIZE_NONE,
		// .validate = true,
	});

	auto scratch = acquire_scratch_arena();
	print_shader_log_message(scratch, options->filename, "glslang SPIR-V generation log", LOG_WARN,
		glslang_program_SPIRV_get_messages(prog));
	release_scratch_arena(scratch);

	uint32_t spirv_words = glslang_program_SPIRV_get_size(prog);
	auto spirv = ARENA_ALLOC_ARRAY(arena, spirv_words, uint32_t);
	glslang_program_SPIRV_get(prog, spirv);

	*out = (ShaderSource) {
		.content = (const char*)spirv,
		.content_size = spirv_words * sizeof(uint32_t),
		.entrypoint = marena_strdup(arena, in->entrypoint),
		.lang = {
			.lang = SHLANG_SPIRV,
			.spirv.target = options->target,
		},
		.stage = in->stage,
	};

	glslang_shader_delete(shader);
	glslang_program_delete(prog);

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

		spvc_set active_vars;
		SPVCCALL(spvc_compiler_get_active_interface_variables(rctx.spvc.compiler, &active_vars));
		SPVCCALL(spvc_compiler_create_shader_resources_for_active_variables(rctx.spvc.compiler, &rctx.spvc.res,  active_vars));
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

	spvc_set active_vars;
	SPVCCALL(spvc_compiler_get_active_interface_variables(rctx.spvc.compiler, &active_vars));
	SPVCCALL(spvc_compiler_create_shader_resources_for_active_variables(rctx.spvc.compiler, &rctx.spvc.res,  active_vars));

	SPVCCALL(_spirv_reflect_all(&rctx));

	spvc_context_destroy(rctx.spvc.ctx);

	shader_reflection_logdump(rctx.reflection);

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
