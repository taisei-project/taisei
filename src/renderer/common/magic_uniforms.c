/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "magic_uniforms.h"

MagicUniformSpec magic_unfiroms[NUM_MAGIC_UNIFORMS] = {
	[UMAGIC_MATRIX_MV]       = { "r_modelViewMatrix",     "mat4", UNIFORM_MAT4 },
	[UMAGIC_MATRIX_PROJ]     = { "r_projectionMatrix",    "mat4", UNIFORM_MAT4 },
	[UMAGIC_MATRIX_TEX]      = { "r_textureMatrix",       "mat4", UNIFORM_MAT4 },
	[UMAGIC_COLOR]           = { "r_color",               "vec4", UNIFORM_VEC4 },
	[UMAGIC_VIEWPORT]        = { "r_viewport",            "vec4", UNIFORM_VEC4 },
	[UMAGIC_COLOR_OUT_SIZES] = { "r_colorOutputSizes",    "vec2", UNIFORM_VEC2 },
	[UMAGIC_DEPTH_OUT_SIZE]  = { "r_depthOutputSize",     "vec2", UNIFORM_VEC2 },
};
