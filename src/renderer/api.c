/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "api.h"
#include "recolor.h"

#include <stdalign.h>

static struct {
	VertexBuffer models_vbuf;
} R;

void r_init(void) {
	_r_init();
	VertexAttribFormat fmt[3];

	r_vertex_attrib_format_interleaved(3, (VertexAttribSpec[]) {
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // position
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // normal
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT }, // texcoord
	}, fmt);

	r_vertex_buffer_create(&R.models_vbuf, 8192 * sizeof(StaticModelVertex), 3, fmt);

	StaticModelVertex quad[] = {
		{ { -0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 0, 0 } },
		{ { -0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 0, 1 } },
		{ {  0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 1, 1 } },
		{ {  0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 1, 0 } },
	};

	r_vertex_buffer_append(&R.models_vbuf, sizeof(quad), quad);
	r_vertex_buffer(&R.models_vbuf);

	recolor_init();
}

void r_shutdown(void) {
	r_vertex_buffer_destroy(&R.models_vbuf);
	_r_shutdown();
}

VertexBuffer* r_vertex_buffer_static_models(void) {
	return &R.models_vbuf;
}

void r_draw_quad(void) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.models_vbuf);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, 0);
	r_vertex_buffer(vbuf_saved);
}

void r_draw_quad_instanced(uint instances) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.models_vbuf);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, instances);
	r_vertex_buffer(vbuf_saved);
}

void r_draw_model_ptr(Model *model) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.models_vbuf);
	r_mat_mode(MM_TEXTURE);
	r_mat_scale(1, -1, 1); // XXX: flipped texture workaround. can we get rid of this somehow?
	r_draw(PRIM_TRIANGLES, 0, model->icount, model->indices, 0);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);
	r_vertex_buffer(vbuf_saved);
}

#ifdef DEBUG
static void attr_unused validate_blend_factor(BlendFactor factor) {
	switch(factor) {
		case BLENDFACTOR_DST_ALPHA:
		case BLENDFACTOR_INV_DST_ALPHA:
		case BLENDFACTOR_DST_COLOR:
		case BLENDFACTOR_INV_DST_COLOR:
		case BLENDFACTOR_INV_SRC_ALPHA:
		case BLENDFACTOR_INV_SRC_COLOR:
		case BLENDFACTOR_ONE:
		case BLENDFACTOR_SRC_ALPHA:
		case BLENDFACTOR_SRC_COLOR:
		case BLENDFACTOR_ZERO:
			return;

		default: UNREACHABLE;
	}
}

static void attr_unused validate_blend_op(BlendOp op) {
	switch(op) {
		case BLENDOP_ADD:
		case BLENDOP_MAX:
		case BLENDOP_MIN:
		case BLENDOP_REV_SUB:
		case BLENDOP_SUB:
			return;

		default: UNREACHABLE;
	}
}
#else
#define validate_blend_factor(x)
#define validate_blend_op(x)
#endif

BlendMode r_blend_compose(
	BlendFactor src_color, BlendFactor dst_color, BlendOp color_op,
	BlendFactor src_alpha, BlendFactor dst_alpha, BlendOp alpha_op
) {
	validate_blend_factor(src_color);
	validate_blend_factor(dst_color);
	validate_blend_factor(src_alpha);
	validate_blend_factor(dst_alpha);
	validate_blend_op(color_op);
	validate_blend_op(alpha_op);

	return BLENDMODE_COMPOSE(
		src_color, dst_color, color_op,
		src_alpha, dst_alpha, alpha_op
	);
}

uint32_t r_blend_component(BlendMode mode, BlendModeComponent component) {
#ifdef DEBUG
	uint32_t result = 0;

	switch(component) {
		case BLENDCOMP_ALPHA_OP:
		case BLENDCOMP_COLOR_OP:
			result = BLENDMODE_COMPONENT(mode, component);
			validate_blend_op(result);
			return result;

		case BLENDCOMP_DST_ALPHA:
		case BLENDCOMP_DST_COLOR:
		case BLENDCOMP_SRC_ALPHA:
		case BLENDCOMP_SRC_COLOR:
			result = BLENDMODE_COMPONENT(mode, component);
			validate_blend_factor(result);
			return result;

		default: UNREACHABLE;
	}
#else
	return BLENDMODE_COMPONENT(mode, component);
#endif
}

void r_blend_unpack(BlendMode mode, UnpackedBlendMode *dest) {
	dest->color.op  = r_blend_component(mode, BLENDCOMP_COLOR_OP);
	dest->color.src = r_blend_component(mode, BLENDCOMP_SRC_COLOR);
	dest->color.dst = r_blend_component(mode, BLENDCOMP_DST_COLOR);
	dest->alpha.op  = r_blend_component(mode, BLENDCOMP_ALPHA_OP);
	dest->alpha.src = r_blend_component(mode, BLENDCOMP_SRC_ALPHA);
	dest->alpha.dst = r_blend_component(mode, BLENDCOMP_DST_ALPHA);
}

const UniformTypeInfo* r_uniform_type_info(UniformType type) {
	static UniformTypeInfo uniform_typemap[] = {
		[UNIFORM_FLOAT]   = {  1, sizeof(float) },
		[UNIFORM_VEC2]    = {  2, sizeof(float) },
		[UNIFORM_VEC3]    = {  3, sizeof(float) },
		[UNIFORM_VEC4]    = {  4, sizeof(float) },
		[UNIFORM_INT]     = {  1, sizeof(int)   },
		[UNIFORM_IVEC2]   = {  2, sizeof(int)   },
		[UNIFORM_IVEC3]   = {  3, sizeof(int)   },
		[UNIFORM_IVEC4]   = {  4, sizeof(int)   },
		[UNIFORM_SAMPLER] = {  1, sizeof(int)   },
		[UNIFORM_MAT3]    = {  9, sizeof(float) },
		[UNIFORM_MAT4]    = { 16, sizeof(float) },
	};

	assert((uint)type < sizeof(uniform_typemap)/sizeof(UniformTypeInfo));
	return uniform_typemap + type;
}

#define VATYPE(type) { sizeof(type), alignof(type) }

static VertexAttribTypeInfo va_type_info[] = {
	[VA_FLOAT]  = VATYPE(float),
	[VA_BYTE]   = VATYPE(int8_t),
	[VA_UBYTE]  = VATYPE(uint8_t),
	[VA_SHORT]  = VATYPE(int16_t),
	[VA_USHORT] = VATYPE(uint16_t),
	[VA_INT]    = VATYPE(int32_t),
	[VA_UINT]   = VATYPE(uint32_t),
};

const VertexAttribTypeInfo* r_vertex_attrib_type_info(VertexAttribType type) {
	uint idx = type;
	assert(idx < sizeof(va_type_info)/sizeof(VertexAttribTypeInfo));
	return va_type_info + idx;
}

void r_vertex_attrib_format_interleaved(size_t nattribs, VertexAttribSpec specs[nattribs], VertexAttribFormat formats[nattribs]) {
	size_t stride = 0;
	memset(formats, 0, sizeof(VertexAttribFormat) * nattribs);

	for(uint i = 0; i < nattribs; ++i) {
		VertexAttribFormat *a = formats + i;
		memcpy(&a->spec, specs + i, sizeof(a->spec));
		const VertexAttribTypeInfo *tinfo = r_vertex_attrib_type_info(a->spec.type);

		assert(a->spec.elements > 0 && a->spec.elements <= 4);

		size_t al = tinfo->alignment;
		size_t misalign = (al - (stride & (al - 1))) & (al - 1);

		stride += misalign;
		a->offset = stride;
		stride += tinfo->size * a->spec.elements;
	}

	for(uint i = 0; i < nattribs; ++i) {
		formats[i].stride = stride;
	}
}
