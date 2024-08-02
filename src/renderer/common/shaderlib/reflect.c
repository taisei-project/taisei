/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "reflect.h"
#include "lang_spirv_private.h"

#include "util.h"

uint shader_basetype_size(ShaderBaseType base_type) {
	switch(base_type) {
		case SHADER_BASETYPE_UNKNOWN:
		case SHADER_BASETYPE_VOID:
		case SHADER_BASETYPE_ATOMIC_COUNTER:
		case SHADER_BASETYPE_STRUCT:
		case SHADER_BASETYPE_IMAGE:
		case SHADER_BASETYPE_SAMPLED_IMAGE:
		case SHADER_BASETYPE_SAMPLER:
		case SHADER_BASETYPE_ACCELERATION_STRUCTURE:
			return 0;  // Unknown/opaque

		case SHADER_BASETYPE_BOOLEAN:  // FIXME is this correct for booleans?
		case SHADER_BASETYPE_INT8:
		case SHADER_BASETYPE_UINT8:
			return 1;

		case SHADER_BASETYPE_INT16:
		case SHADER_BASETYPE_UINT16:
		case SHADER_BASETYPE_FP16:
			return 2;

		case SHADER_BASETYPE_INT32:
		case SHADER_BASETYPE_UINT32:
		case SHADER_BASETYPE_FP32:
			return 4;

		case SHADER_BASETYPE_INT64:
		case SHADER_BASETYPE_UINT64:
		case SHADER_BASETYPE_FP64:
			return 8;
	}

	UNREACHABLE;
}

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

uint shader_type_size(const ShaderDataType *type) {
	if(type->array_stride) {
		return type->array_stride * type->array_size;
	} else if(type->matrix_stride) {
		return type->matrix_stride * type->matrix_columns;
	} else {
		return shader_basetype_size(type->base_type) * type->vector_size;
	}
}

static uint shader_type_unpack_from_bytes_32bit(
	const ShaderDataType *type,
	uint src_words, const uint32_t src[src_words],
	uint dst_words, uint32_t dst[dst_words]
) {
	uint unit_size = type->vector_size;
	uint units_per_element = 1;
	uint unit_stride = unit_size;
	uint num_elements = 1;
	bool has_holes = false;

	if(type->matrix_stride) {
		unit_stride = type->matrix_stride / 4;

		if(unit_size == unit_stride) {
			unit_size *= type->matrix_columns;
			unit_stride = unit_size;
		} else {
			units_per_element = type->matrix_columns;
			has_holes = true;
		}
	}

	uint src_element_size = unit_size * units_per_element;
	uint dst_element_size = unit_stride * units_per_element;

	if(type->array_stride) {
		num_elements = type->array_size;

		if(type->array_stride != 4 * dst_element_size) {
			assume(units_per_element == 1);
			assume(dst_element_size == unit_stride);
			assume(type->array_stride / 4 > unit_stride);
			unit_stride = type->array_stride / 4;
			dst_element_size = unit_stride * units_per_element;
			has_holes = true;
		}

		assume(type->array_stride == 4 * dst_element_size);
	}

	if(has_holes) {
		assume(unit_size != unit_stride);
		assume(src_element_size != dst_element_size);

		uint elems_copied = 0;

		while(
			src_words >= src_element_size &&
			dst_words >= dst_element_size &&
			elems_copied < num_elements
		) {
			for(uint i = 0; i < units_per_element; ++i) {
				memcpy(dst, src, sizeof(*dst) * unit_size);
				src += unit_size;
				src_words -= unit_size;
				dst += unit_stride;
				dst_words -= unit_stride;
			}

			++elems_copied;
		}

		return elems_copied;
	} else {
		assume(unit_size == unit_stride);
		assume(src_element_size == dst_element_size);

		uint elems_to_copy = min(num_elements, min(src_words, dst_words) / src_element_size);
		memcpy(dst, src, sizeof(*dst) * src_element_size * elems_to_copy);
		return elems_to_copy;
	}
}

uint shader_type_unpack_from_bytes(
	const ShaderDataType *type,
	uint src_size, const void *src,
	uint dst_size, void *dst
) {
	switch(shader_basetype_size(type->base_type)) {
		case 0: return 0;
		case 4:	return shader_type_unpack_from_bytes_32bit(
			type,
			src_size / 4, src,
			dst_size / 4, dst);

		case 8: UNREACHABLE;  /* We don't use 64-bit types (TODO?) */
	}

	UNREACHABLE;
}

static const char *shader_basetype_string(ShaderBaseType base_type) {
	switch(base_type) {
        case SHADER_BASETYPE_VOID:			return "void";
        case SHADER_BASETYPE_BOOLEAN:		return "bool";
        case SHADER_BASETYPE_INT8:			return "int8_t";
        case SHADER_BASETYPE_UINT8:			return "uint8_t";
        case SHADER_BASETYPE_INT16:			return "int16_t";
        case SHADER_BASETYPE_UINT16:		return "uint16_t";
        case SHADER_BASETYPE_INT32:			return "int";
        case SHADER_BASETYPE_UINT32:		return "uint32_t";
        case SHADER_BASETYPE_INT64:			return "int64_t";
        case SHADER_BASETYPE_UINT64:		return "uint64_t";
        case SHADER_BASETYPE_FP16:			return "half";
        case SHADER_BASETYPE_FP32:			return "float";
        case SHADER_BASETYPE_FP64:			return "double";
        case SHADER_BASETYPE_STRUCT:		return "struct";
        case SHADER_BASETYPE_IMAGE:			return "image";
        case SHADER_BASETYPE_SAMPLED_IMAGE:	return "sampledImage";
        case SHADER_BASETYPE_SAMPLER:		return "sampler";
		default:							return "unknown";
	}
}

static void shader_struct_field_dump_string(ShaderStructField *field, StringBuffer *buf) {
	strbuf_cat(buf, shader_basetype_string(field->type.base_type));

	// HLSL-style type names are easier to format

	if(field->type.vector_size > 1 || field->type.matrix_columns > 1) {
		strbuf_printf(buf, "%u", field->type.vector_size);
	}

	if(field->type.matrix_columns > 1) {
		strbuf_printf(buf, "x%u", field->type.matrix_columns);
	}

	if(field->type.array_size > 0) {
		strbuf_printf(buf, " %s[%u]", field->name, field->type.array_size);
	} else {
		strbuf_printf(buf, " %s", field->name);
	}
}

static void shader_sampler_dump_string(ShaderSampler *sampler, StringBuffer *buf) {
	strbuf_cat(buf, "sampler");

	const char *dim_str;

	switch(sampler->type.dim) {
		case SHADER_SAMPLER_DIM_1D:		dim_str = "1D"; break;
		case SHADER_SAMPLER_DIM_2D:		dim_str = "2D"; break;
		case SHADER_SAMPLER_DIM_3D:		dim_str = "3D"; break;
		case SHADER_SAMPLER_DIM_CUBE:	dim_str = "Cube"; break;
		case SHADER_SAMPLER_DIM_BUFFER:	dim_str = "Buffer"; break;
		default:						dim_str = "WTF"; break;
	}

	strbuf_cat(buf, dim_str);

	if(sampler->type.is_multisampled) {
		strbuf_cat(buf, "MS");
	}

	if(sampler->type.is_arrayed) {
		strbuf_cat(buf, "Array");
	}

	if(sampler->type.is_depth) {
		strbuf_cat(buf, "Shadow");
	}

	if(sampler->array_size) {
		strbuf_printf(buf, " %s[%d]", sampler->name, sampler->array_size);
	} else {
		strbuf_printf(buf, " %s", sampler->name);
	}
}

ShaderReflection *shader_source_reflect(const ShaderSource *src, MemArena *arena) {
	ShaderReflection *r = NULL;

	switch(src->lang.lang) {
		case SHLANG_SPIRV:
			r = _spirv_reflect(src, arena);
			break;

		default:
			log_error("Reflection not supported for this language");
			return NULL;
	}

	if(UNLIKELY(!r)) {
		return NULL;
	}

	StringBuffer buf = {};

	log_debug("%u uniform buffers", r->num_uniform_buffers);

	for(uint i = 0; i < r->num_uniform_buffers; ++i) {
		auto ubuf = &r->uniform_buffers[i];

		log_debug("Uniform buffer #%d (set=%d, binding=%d): %s (%d bytes)",
				  i, ubuf->set, ubuf->binding, ubuf->name, ubuf->size);

		for(uint f = 0; f < ubuf->num_fields; ++f) {
			auto field = &ubuf->fields[f];
			strbuf_clear(&buf);
			shader_struct_field_dump_string(field, &buf);
			log_debug("Field #%d (%+6d):  %s", f, field->offset, buf.start);
		}
	}

	log_debug("%u samplers", r->num_samplers);

	for(uint i = 0; i < r->num_samplers; ++i) {
		auto sampler = &r->samplers[i];

		strbuf_clear(&buf);
		shader_sampler_dump_string(sampler, &buf);

		log_debug("Sampler #%d (set=%d, binding=%d):  %s",
				  i, sampler->set, sampler->binding, buf.start);
	}

	strbuf_free(&buf);
	return r;
}
