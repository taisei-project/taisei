/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_reflect.h"

#include "util.h"

UniformType shader_type_to_uniform_type(const ShaderDataType *type) {
	static const struct uniform_type_map_entry {
		ShaderBaseType base_type;
		uint vector_size;
		uint columns;
	} uniform_type_map[] = {
		[UNIFORM_FLOAT] = { SHADER_BASETYPE_FP32,  1, 1 },
		[UNIFORM_VEC2]  = { SHADER_BASETYPE_FP32,  2, 1 },
		[UNIFORM_VEC3]  = { SHADER_BASETYPE_FP32,  3, 1 },
		[UNIFORM_VEC4]  = { SHADER_BASETYPE_FP32,  4, 1 },
		[UNIFORM_INT]   = { SHADER_BASETYPE_INT32, 1, 1 },
		[UNIFORM_IVEC2] = { SHADER_BASETYPE_INT32, 2, 1 },
		[UNIFORM_IVEC3] = { SHADER_BASETYPE_INT32, 3, 1 },
		[UNIFORM_IVEC4] = { SHADER_BASETYPE_INT32, 4, 1 },
		[UNIFORM_MAT3]  = { SHADER_BASETYPE_FP32,  3, 3 },
		[UNIFORM_MAT4]  = { SHADER_BASETYPE_FP32,  4, 4 },
	};

	for(uint i = 0; i < ARRAY_SIZE(uniform_type_map); ++i) {
		auto m = &uniform_type_map[i];
		if(
			m->base_type == type->base_type &&
			m->vector_size == type->vector_size &&
			m->columns == type->matrix_columns
		) {
			return (UniformType)i;
		}
	}

	return UNIFORM_UNKNOWN;
}
