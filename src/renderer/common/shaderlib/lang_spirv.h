/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"

#include "memory/arena.h"

typedef enum SPIRVTarget {
	SPIRV_TARGET_OPENGL_450,
	SPIRV_TARGET_VULKAN_10,
	SPIRV_TARGET_VULKAN_11,
} SPIRVTarget;

typedef enum SPIRVOptimizationLevel {
	SPIRV_OPTIMIZE_NONE,
	SPIRV_OPTIMIZE_SIZE,
	SPIRV_OPTIMIZE_PERFORMANCE,
} SPIRVOptimizationLevel;

typedef struct ShaderLangInfoSPIRV {
	SPIRVTarget target;
} ShaderLangInfoSPIRV;

typedef enum SPIRVCompileFlag {
	SPIRV_CFLAG_DEBUG_INFO = (1 << 0),
	SPIRV_CFLAG_VULKAN_RELAXED = (1 << 1),
} SPIRVCompileFlag;

typedef struct SPIRVCompileOptions {
	ShaderMacro *macros;
	const char *filename;
	SPIRVTarget target;
	SPIRVOptimizationLevel optimization_level;
	uint8_t flags;
} SPIRVCompileOptions;

typedef struct SPIRVDecompileOptions {
	const ShaderLangInfo *lang;
	bool reflect;

	union {
		struct {
			bool vulkan_semantics;
		} glsl;
	};
} SPIRVDecompileOptions;

typedef struct SPIRVTranspileOptions {
	SPIRVCompileOptions compile;
	SPIRVDecompileOptions decompile;
} SPIRVTranspileOptions;

void spirv_init_compiler(void);
void spirv_shutdown_compiler(void);

bool spirv_transpile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVTranspileOptions *options
) attr_nonnull(1, 2, 3);

bool spirv_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVCompileOptions *options,
	bool reflect
) attr_nonnull_all;

bool spirv_decompile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVDecompileOptions *options
) attr_nonnull_all;
