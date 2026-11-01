/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"

#include "memory/arena.h"

typedef enum WGSLTranslateFlag {
	WGSL_TFLAG_ALLOW_NON_UNIFORM_DERIVATIVES          = (1 << 0),
	WGSL_TFLAG_ALLOW_NON_UNIFORM_SUBGROUP_OPERATIONS  = (1 << 1),
	WGSL_TFLAG_DISABLE_UNREACHABLE_CODE_WARNING       = (1 << 2),
	WGSL_TFLAG_MINIFY                                 = (1 << 3),
} WGSLTranslateFlag;

typedef struct WGSLTranslateOptions {
	uint8_t flags;
} WGSLTranslateOptions;

bool wgsl_supported(void);

bool wgsl_translate_from_spirv(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const WGSLTranslateOptions *options
) attr_nonnull(1, 2, 3);
