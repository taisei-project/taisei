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

	if(sampler->type.flags & SHADER_SAMPLER_MULTISAMPLED) {
		strbuf_cat(buf, "MS");
	}

	if(sampler->type.flags & SHADER_SAMPLER_ARRAYED) {
		strbuf_cat(buf, "Array");
	}

	if(sampler->type.flags & SHADER_SAMPLER_DEPTH) {
		strbuf_cat(buf, "Shadow");
	}

	if(sampler->array_size) {
		strbuf_printf(buf, " %s[%d]", sampler->name, sampler->array_size);
	} else {
		strbuf_printf(buf, " %s", sampler->name);
	}
}

void shader_reflection_logdump(const ShaderReflection *reflect) {
	StringBuffer buf = {};

	log_debug("%u uniform buffers", reflect->num_uniform_buffers);

	for(uint i = 0; i < reflect->num_uniform_buffers; ++i) {
		auto ubuf = &reflect->uniform_buffers[i];

		log_debug("Uniform buffer #%d (set=%d, binding=%d): %s (%d bytes)",
				  i, ubuf->set, ubuf->binding, ubuf->name, ubuf->size);

		for(uint f = 0; f < ubuf->num_fields; ++f) {
			auto field = &ubuf->fields[f];
			strbuf_clear(&buf);
			shader_struct_field_dump_string(field, &buf);
			log_debug("Field #%d (%+6d):  %s", f, field->offset, buf.start);
		}
	}

	log_debug("%u samplers", reflect->num_samplers);

	for(uint i = 0; i < reflect->num_samplers; ++i) {
		auto sampler = &reflect->samplers[i];

		strbuf_clear(&buf);
		shader_sampler_dump_string(sampler, &buf);

		log_debug("Sampler #%d (set=%d, binding=%d):  %s",
				  i, sampler->set, sampler->binding, buf.start);
	}

	log_debug("%u inputs", reflect->num_inputs);

	for(uint i = 0; i < reflect->num_inputs; ++i) {
		attr_unused auto input = &reflect->inputs[i];
		log_debug("Input #%d: (location = %d, consumes = %d) %s",
			      i, input->location, input->num_locations_consumed, input->name);
	}

	log_debug("Used input locations map: 0b%llb", (unsigned long long)reflect->used_input_locations_map);

	strbuf_free(&buf);
}

ShaderReflection *shader_source_reflect(const ShaderSource *src, MemArena *arena) {
	ShaderReflection *r = NULL;

	switch(src->lang.lang) {
		case SHLANG_SPIRV:
			r = _spirv_reflect(src, arena);
			break;

		default:
			log_error("Reflection not supported for %s", shader_lang_name(src->lang.lang));
			return NULL;
	}

	if(UNLIKELY(!r)) {
		return NULL;
	}

	return r;
}

#define REFLECT_SERIALIZE_VERSION 1

#define CHECK_LIMIT(_value, _limit, _name) ({ \
	if((_value) > (_limit)) { \
		log_error("Too many " _name ": %u >= %u", (_value), (_limit)); \
		goto fail; \
	} \
	(void)0; \
})

#define MAX_UNIFORM_BUFFERS 16
#define MAX_SAMPLERS 64
#define MAX_INPUTS 64
#define MAX_BLOCK_FIELDS 128

static bool write_string(const char *s, SDL_IOStream *dest) {
	size_t len = strlen(s);

	if(len > 255) {
		log_error("String is too long (%zu bytes)", len);
		return false;
	}

	SDL_WriteU8(dest, len & 0xff);
	SDL_WriteIO(dest, s, len);

	return true;
}

static char *read_string(MemArena *arena, SDL_IOStream *src) {
	uint8_t len = 0;
	SDL_ReadU8(src, &len);
	char *buf = marena_alloc(arena, len + 1);
	SDL_ReadIO(src, buf, len);
	buf[len] = 0;
	return buf;
}

static bool write_shader_data_type(const ShaderDataType *type, SDL_IOStream *output) {
	SDL_WriteU16LE(output, type->base_type);
	SDL_WriteU32LE(output, type->vector_size);
	SDL_WriteU32LE(output, type->array_size);
	SDL_WriteU32LE(output, type->array_stride);
	SDL_WriteU32LE(output, type->matrix_columns);
	SDL_WriteU32LE(output, type->matrix_stride);
	return true;
}

static bool read_shader_data_type(ShaderDataType *type, SDL_IOStream *input) {
	uint16_t base_type;
	SDL_ReadU16LE(input, &base_type); type->base_type = base_type;
	SDL_ReadU32LE(input, &type->vector_size);
	SDL_ReadU32LE(input, &type->array_size);
	SDL_ReadU32LE(input, &type->array_stride);
	SDL_ReadU32LE(input, &type->matrix_columns);
	SDL_ReadU32LE(input, &type->matrix_stride);
	return true;
}

static bool write_shader_struct_field(const ShaderStructField *field, SDL_IOStream *output) {
	if(!write_string(field->name, output)) {
		return false;
	}

	SDL_WriteU32LE(output, field->offset);

	return write_shader_data_type(&field->type, output);
}

static bool read_shader_struct_field(ShaderStructField *field, MemArena *arena, SDL_IOStream *input) {
	if(!(field->name = read_string(arena, input))) {
		return false;
	}

	SDL_ReadU32LE(input, &field->offset);

	return read_shader_data_type(&field->type, input);
}

static bool write_shader_block(const ShaderBlock *block, SDL_IOStream *output) {
	if(!write_string(block->name, output)) {
		return false;
	}

	SDL_WriteU32LE(output, block->set);
	SDL_WriteU32LE(output, block->binding);
	SDL_WriteU32LE(output, block->size);
	SDL_WriteU32LE(output, block->num_fields);
	CHECK_LIMIT(block->num_fields, MAX_BLOCK_FIELDS, "block fields");

	for(uint i = 0; i < block->num_fields; ++i) {
		if(!write_shader_struct_field(block->fields + i, output)) {
			return false;
		}
	}

	return true;

fail:
	return false;
}

static bool read_shader_block(ShaderBlock *block, MemArena *arena, SDL_IOStream *input) {
	if(!(block->name = read_string(arena, input))) {
		return false;
	}

	SDL_ReadU32LE(input, &block->set);
	SDL_ReadU32LE(input, &block->binding);
	SDL_ReadU32LE(input, &block->size);
	SDL_ReadU32LE(input, &block->num_fields);
	CHECK_LIMIT(block->num_fields, MAX_BLOCK_FIELDS, "block fields");
	block->fields = ARENA_ALLOC_ARRAY(arena, block->num_fields, typeof(*block->fields));

	for(uint i = 0; i < block->num_fields; ++i) {
		if(!read_shader_struct_field(block->fields + i, arena, input)) {
			return false;
		}
	}

	return true;

fail:
	return false;
}

static bool write_shader_sampler_type(const ShaderSamplerType *sampler_type, SDL_IOStream *output) {
	SDL_WriteU16LE(output, sampler_type->dim);
	SDL_WriteU16LE(output, sampler_type->flags);
	return true;
}

static bool read_shader_sampler_type(ShaderSamplerType *sampler_type, SDL_IOStream *input) {
	uint16_t dim;
	SDL_ReadU16LE(input, &dim); sampler_type->dim = dim;
	SDL_ReadU16LE(input, &sampler_type->flags);
	return true;
}

static bool write_shader_sampler(const ShaderSampler *sampler, SDL_IOStream *output) {
	if(!write_string(sampler->name, output)) {
		return false;
	}

	if(!write_shader_sampler_type(&sampler->type, output)) {
		return false;
	}

	SDL_WriteU32LE(output, sampler->set);
	SDL_WriteU32LE(output, sampler->binding);
	SDL_WriteU32LE(output, sampler->array_size);

	return true;
}

static bool read_shader_sampler(ShaderSampler *sampler, MemArena *arena, SDL_IOStream *input) {
	if(!(sampler->name = read_string(arena, input))) {
		return false;
	}

	if(!read_shader_sampler_type(&sampler->type, input)) {
		return false;
	}

	SDL_ReadU32LE(input, &sampler->set);
	SDL_ReadU32LE(input, &sampler->binding);
	SDL_ReadU32LE(input, &sampler->array_size);

	return true;
}

static bool write_shader_input(const ShaderInput *shinput, SDL_IOStream *output) {
	if(!write_string(shinput->name, output)) {
		return false;
	}

	SDL_WriteU32LE(output, shinput->location);
	SDL_WriteU32LE(output, shinput->num_locations_consumed);

	return true;
}

static bool read_shader_input(ShaderInput *shinput, MemArena *arena, SDL_IOStream *input) {
	if(!(shinput->name = read_string(arena, input))) {
		return false;
	}

	SDL_ReadU32LE(input, &shinput->location);
	SDL_ReadU32LE(input, &shinput->num_locations_consumed);

	return true;
}

bool shader_reflection_serialize(const ShaderReflection *r, SDL_IOStream *output) {
	if(!r) {
		SDL_WriteU16LE(output, 0);
		return true;
	}

	SDL_WriteU16LE(output, REFLECT_SERIALIZE_VERSION);

	SDL_WriteU32LE(output, r->num_uniform_buffers);
	CHECK_LIMIT(r->num_uniform_buffers, MAX_UNIFORM_BUFFERS, "uniform buffers");

	for(uint i = 0; i < r->num_uniform_buffers; ++i) {
		if(!write_shader_block(r->uniform_buffers + i, output)) {
			return false;
		}
	}

	SDL_WriteU32LE(output, r->num_samplers);
	CHECK_LIMIT(r->num_samplers, MAX_SAMPLERS, "samplers");

	for(uint i = 0; i < r->num_samplers; ++i) {
		if(!write_shader_sampler(r->samplers + i, output)) {
			return false;
		}
	}

	SDL_WriteU32LE(output, r->num_inputs);
	CHECK_LIMIT(r->num_inputs, MAX_INPUTS, "inputs");
	SDL_WriteU64LE(output, r->used_input_locations_map);

	for(uint i = 0; i < r->num_inputs; ++i) {
		if(!write_shader_input(r->inputs + i, output)) {
			return false;
		}
	}

	return true;

fail:
	return false;
}

bool shader_reflection_deserialize(const ShaderReflection **out_reflect, MemArena *arena, SDL_IOStream *input) {
	uint16_t version = 0xffff;

	SDL_ReadU16LE(input, &version);

	if(version == 0) {
		*out_reflect = NULL;
		return true;
	}

	if(version != REFLECT_SERIALIZE_VERSION) {
		log_error("Can't handle reflection format version %u", version);
		return false;
	}

	auto arena_snap = marena_snapshot(arena);
	auto r = ARENA_ALLOC(arena, ShaderReflection, {});

	SDL_ReadU32LE(input, &r->num_uniform_buffers);
	CHECK_LIMIT(r->num_uniform_buffers, MAX_UNIFORM_BUFFERS, "uniform buffers");
	r->uniform_buffers = ARENA_ALLOC_ARRAY(arena, r->num_uniform_buffers, typeof(*r->uniform_buffers));

	for(uint i = 0; i < r->num_uniform_buffers; ++i) {
		if(!read_shader_block(r->uniform_buffers + i, arena, input)) {
			goto fail;
		}
	}

	SDL_ReadU32LE(input, &r->num_samplers);
	CHECK_LIMIT(r->num_samplers, MAX_SAMPLERS, "samplers");
	r->samplers = ARENA_ALLOC_ARRAY(arena, r->num_samplers, typeof(*r->samplers));

	for(uint i = 0; i < r->num_samplers; ++i) {
		if(!read_shader_sampler(r->samplers + i, arena, input)) {
			goto fail;
		}
	}

	SDL_ReadU32LE(input, &r->num_inputs);
	CHECK_LIMIT(r->num_inputs, MAX_INPUTS, "inputs");
	r->inputs = ARENA_ALLOC_ARRAY(arena, r->num_inputs, typeof(*r->inputs));
	SDL_ReadU64LE(input, &r->used_input_locations_map);

	for(uint i = 0; i < r->num_inputs; ++i) {
		if(!read_shader_input(r->inputs + i, arena, input)) {
			goto fail;
		}
	}

	*out_reflect = r;
	return true;

fail:
	marena_rollback(arena, &arena_snap);
	return NULL;
}
