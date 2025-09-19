/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "lang_spirv.h"
#include "reflect.h"

#include "memory/arena.h"

bool _spirv_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVCompileOptions *options
) attr_nonnull(1, 2, 3);

bool _spirv_decompile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVDecompileOptions *options
) attr_nonnull(1, 2, 3);

ShaderReflection *_spirv_reflect(const ShaderSource *src, MemArena *arena) attr_nonnull(1, 2);
