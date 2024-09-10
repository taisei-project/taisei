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

typedef struct ShaderLangInfoDXBC {
	uint shader_model;
} ShaderLangInfoDXBC;

typedef struct DXBCCompileOptions {
	const char *entrypoint;
} DXBCCompileOptions;

bool dxbc_init_compiler(void);
void dxbc_shutdown_compiler(void);

bool dxbc_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const DXBCCompileOptions *options
) attr_nonnull(1, 2, 3);
