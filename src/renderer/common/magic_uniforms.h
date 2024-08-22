/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

typedef enum MagicUniformIndex {
	UMAGIC_INVALID = -1,

	UMAGIC_MATRIX_MV,
	UMAGIC_MATRIX_PROJ,
	UMAGIC_MATRIX_TEX,
	UMAGIC_COLOR,
	UMAGIC_VIEWPORT,
	UMAGIC_COLOR_OUT_SIZES,
	UMAGIC_DEPTH_OUT_SIZE,

	NUM_MAGIC_UNIFORMS,
} MagicUniformIndex;

typedef struct MagicUniformSpec {
	const char *name;
	const char *typename;
	UniformType type;
} MagicUniformSpec;

extern MagicUniformSpec magic_unfiroms[NUM_MAGIC_UNIFORMS];
