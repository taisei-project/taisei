/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_wgsl.h"
#include "shaderlib.h"

#include "log.h"

#include <wgslcrap.h>
#include <spirv-tools/libspirv.h>

static void *alloc_arena_fn(size_t size, void *arg) {
	MemArena *arena = arg;
	return marena_alloc(arena, size);
}

enum {
	SPIRV_HEADER_WORDS = 5,
};

enum SPIRVOp {
	SPV_OP_SourceContinued = 2,
	SPV_OP_Source = 3,
	SPV_OP_SourceExtension = 4,
	SPV_OP_Name = 5,
	SPV_OP_MemberName = 6,
	SPV_OP_String = 7,
	SPV_OP_Line = 8,
	SPV_OP_NoLine = 317,
};

static bool should_strip(uint32_t opcode) {
	switch((enum SPIRVOp)opcode) {
		case SPV_OP_SourceContinued:
		case SPV_OP_Source:
		case SPV_OP_SourceExtension:
		case SPV_OP_Line:
		case SPV_OP_NoLine:
		// Let's keep these for a slightly more readable output.
		// case SPV_OP_Name:
		// case SPV_OP_MemberName:
		// case SPV_OP_String:
			return true;

		default: return false;
	}
}

static uint32_t spirv_strip_debug(uint32_t word_count, uint32_t spirv[word_count]) {
	uint32_t read_idx = SPIRV_HEADER_WORDS;
	uint32_t write_idx = SPIRV_HEADER_WORDS;

	while(read_idx < word_count) {
		uint32_t inst = spirv[read_idx];

		uint32_t opcode = inst & 0xFFFF;
		uint32_t count = inst >> 16;

		if(count == 0 || read_idx + count > word_count) {
			log_error("Malformed SPIR-V instruction at %u", read_idx);
			return 0;
		}

		if(!should_strip(opcode)) {
			for(uint32_t i = 0; i < count; ++i) {
				spirv[write_idx++] = spirv[read_idx + i];
			}
		}

		read_idx += count;
	}

	return write_idx;
}

static void spirv_opt_message(
	spv_message_level_t level,
	const char *source,
	const spv_position_t *position,
	const char *message
) {
	LogLevel l = LOG_INFO;

	switch(level) {
		case SPV_MSG_FATAL:
		case SPV_MSG_INTERNAL_ERROR:
		case SPV_MSG_ERROR:
			l = LOG_ERROR;
		case SPV_MSG_WARNING:
			l = LOG_WARN;
		case SPV_MSG_INFO:
			l = LOG_INFO;
		case SPV_MSG_DEBUG:
			l = LOG_DEBUG;
		break;
	}

	log_custom(l, "%s", message);
}

bool wgsl_translate_from_spirv(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena
) {
	if(in->lang.lang != SHLANG_SPIRV) {
		log_error("Source is not a SPIR-V shader");
		return false;
	}

	auto snap = marena_snapshot(arena);

	WGSLAllocator alloc = {
		.func = alloc_arena_fn,
		.arg = arena,
	};

	uint32_t *spv = CASTPTR_ASSUME_ALIGNED(in->content, uint32_t);
	uint32_t spv_size = in->content_size / sizeof(*spv);

	// auto scratch = acquire_scratch_arena();

	// Naga chokes on some debug info that glslang emits, so strip it here.
	// We can't disable debug info completely, because it would remove variable names,
	// which Taisei uses for interfacing.
	uint32_t *spv_stripped = marena_memdup(arena, spv, in->content_size);
	uint32_t spv_stripped_size = spirv_strip_debug(spv_size, spv_stripped);

	// We rely on spirv_webgpu_transform::combimgsampsplitter to convert combined image samplers into texture/sampler
	// pairs. Some of our shaders leave some samplers unused, which confuses the transform pass and makes it assign
	// wrong binding indices to separate samplers. We could enable full optimizations at the compilation stage to avoid
	// that problem, but that makes the transform generate invalid SPIR-V in some cases. So we invoke the optimizer
	// here manually, just to strip those unused samplers, without aggressive optimizations.
	//
	// TODO: investigate which spirv-opt pass triggers the bug, send a report to spirv_webgpu_transform

	auto opt = spvOptimizerCreate(SPV_ENV_VULKAN_1_0);

	if(!opt) {
		log_error("spvOptimizerCreate() failed");
		marena_rollback(arena, &snap);
		return false;
	}

	spvOptimizerSetMessageConsumer(opt, spirv_opt_message);

	const char *passes[] = {
		"--remove-unused-interface-variables",
		"--eliminate-dead-variables",
	};

	if(!spvOptimizerRegisterPassesFromFlags(opt, passes, countof(passes))) {
		log_error("spvOptimizerRegisterPassesFromFlags() failed");
		marena_rollback(arena, &snap);
		return false;
	}

	auto options = NOT_NULL(spvOptimizerOptionsCreate());
	spvOptimizerOptionsSetRunValidator(options, true);

	spv_binary opt_binary = NULL;
	auto opt_result = spvOptimizerRun(opt, spv_stripped, spv_stripped_size, &opt_binary, options);
	spvOptimizerOptionsDestroy(options);
	spvOptimizerDestroy(opt);

	marena_rollback(arena, &snap);

	if(opt_result != SPV_SUCCESS) {
		log_error("spvOptimizerRun() failed: code %i", opt_result);
		return false;
	}

	// spirv_to_wgsl() is implemented in Rust, it applies the combimgsampsplitter pass and invokes Naga
	auto result = spirv_to_wgsl(opt_binary->code, opt_binary->wordCount, alloc);
	spvBinaryDestroy(opt_binary);

	if(result.error) {
		assert(!result.content);
		log_error("%s", result.error);
		marena_rollback(arena, &snap);
		return false;
	}

	assume(result.content != NULL);

	// SDL jank: current WebGPU backend tries to scan the shader for bindings.
	// It is very brittle, and breaks when the decorations are not on the same line as the declaration.
	// Work around this by collapsing the next linebreak whenever a @ is detected.

	char *c = result.content;
	for(;;) {
		c = strchr(c, '@');
		if(!c) {
			break;
		}

		char *next_line = strchr(c, '\n');

		if(!next_line) {
			break;
		}

		*next_line = ' ';
		c = next_line + 1;
	}

	*out = (ShaderSource) {
		.lang = {
			.lang = SHLANG_WGSL,
		},
		.content = result.content,
		.content_size = result.content_size,
		.entrypoint = marena_strdup(arena, in->entrypoint),
		.stage = in->stage,
	};

	return true;
}

bool wgsl_supported(void) {
	return true;
}
