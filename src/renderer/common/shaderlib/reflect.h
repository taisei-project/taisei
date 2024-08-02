/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/arena.h"
#include "renderer/api.h"

typedef enum ShaderBaseType {
	// Matches the spvc_basetype enum in SPIRV-Cross for simplicity
	// We only use a subset of these

	SHADER_BASETYPE_UNKNOWN = 0,
	SHADER_BASETYPE_VOID = 1,
	SHADER_BASETYPE_BOOLEAN = 2,
	SHADER_BASETYPE_INT8 = 3,
	SHADER_BASETYPE_UINT8 = 4,
	SHADER_BASETYPE_INT16 = 5,
	SHADER_BASETYPE_UINT16 = 6,
	SHADER_BASETYPE_INT32 = 7,
	SHADER_BASETYPE_UINT32 = 8,
	SHADER_BASETYPE_INT64 = 9,
	SHADER_BASETYPE_UINT64 = 10,
	SHADER_BASETYPE_ATOMIC_COUNTER = 11,
	SHADER_BASETYPE_FP16 = 12,
	SHADER_BASETYPE_FP32 = 13,
	SHADER_BASETYPE_FP64 = 14,
	SHADER_BASETYPE_STRUCT = 15,
	SHADER_BASETYPE_IMAGE = 16,
	SHADER_BASETYPE_SAMPLED_IMAGE = 17,
	SHADER_BASETYPE_SAMPLER = 18,
	SHADER_BASETYPE_ACCELERATION_STRUCTURE = 19,
} ShaderBaseType;

typedef struct ShaderDataType {
	ShaderBaseType base_type;
	uint vector_size;
	uint array_size;  // NOTE: currently folding all dimensions into one
	uint array_stride;
	uint matrix_columns;
	uint matrix_stride;
} ShaderDataType;

typedef struct ShaderStructField {
	const char *name;
	uint offset;
	ShaderDataType type;
} ShaderStructField;

typedef struct ShaderBlock {
	const char *name;
	uint set;
	uint binding;
	uint num_fields;
	uint size;
	ShaderStructField *fields;
} ShaderBlock;

typedef enum ShaderSamplerDimension {
	SHADER_SAMPLER_DIM_UNKNOWN,
	SHADER_SAMPLER_DIM_1D,
	SHADER_SAMPLER_DIM_2D,
	SHADER_SAMPLER_DIM_3D,
	SHADER_SAMPLER_DIM_CUBE,
	SHADER_SAMPLER_DIM_BUFFER,
} ShaderSamplerDimension;

typedef struct ShaderSamplerType {
	ShaderSamplerDimension dim : 16;
	bool is_depth : 1;
	bool is_arrayed : 1;
	bool is_multisampled : 1;
} ShaderSamplerType;

typedef struct ShaderSampler {
	const char *name;
	ShaderSamplerType type;
	uint set;
	uint binding;
	uint array_size;
} ShaderSampler;

typedef struct ShaderReflection {
	uint num_uniform_buffers;
	ShaderBlock *uniform_buffers;

	uint num_samplers;
	ShaderSampler *samplers;
} ShaderReflection;

uint shader_basetype_size(ShaderBaseType base_type);
UniformType shader_type_to_uniform_type(const ShaderDataType *type);
uint shader_type_size(const ShaderDataType *type);

/*
 * Unpacks data from `src` into `dst` according to the ShaderReflectedType spec.
 * The source buffer is assumed to contain a packed stream of elements of the base type.
 * The destination buffer is suitable for copying into a GPU buffer.
 *
 * This function may write less array elements than specified by the type's array_size
 * if either the source or destination buffers are too small, but it will not write
 * partial elements.
 *
 * Returns the number of array elements written. Non-array types count as 1-element arrays.
 */
uint shader_type_unpack_from_bytes(
	const ShaderDataType *type,
	uint src_size, const void *src,
	uint dst_size, void *dst);

ShaderReflection *shader_source_reflect(const ShaderSource *src, MemArena *arena);
