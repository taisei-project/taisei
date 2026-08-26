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

bool wgsl_supported(void);

bool wgsl_translate_from_spirv(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena
) attr_nonnull(1, 2, 3);
