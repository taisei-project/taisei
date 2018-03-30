/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "api.h"
#include "common/matstack.h"
#include "glm.h"

#include <stdalign.h>

#define SPRITE_BATCH_CAPACITY 2048

typedef struct Quad2DVertex {
	float pos[2];
} Quad2DVertex;

typedef struct Sprite2DAttrs {
	mat4 transform;
	float rgba[4];
	float texrect[4];
	float custom;
	uint8_t flip[2];
} Sprite2DAttrs;

static struct {
	struct {
		VertexBuffer models;
		VertexBuffer sprites;
	} vbufs;

	struct {
		Texture *tex;
		ShaderProgram *shader;
		BlendMode blend;
		RenderTarget *render_target;
		CullFaceMode cull_mode;
		DepthTestFunc depth_func;
		uint cull_enabled : 1;
		uint depth_test_enabled : 1;
		uint depth_write_enabled : 1;
		uint num_pending;
	} sprite_batch;

	Quad2DVertex quad_2d[4];
} R = {
	.quad_2d = {
		{ { -0.5, -0.5 } },
		{ { -0.5,  0.5 } },
		{ {  0.5,  0.5 } },
		{ {  0.5, -0.5 } },
	}
};

static void r_init_vbuf_models(VertexBuffer *vbuf) {
	VertexAttribFormat fmt[3];

	r_vertex_attrib_format_interleaved(3, (VertexAttribSpec[]) {
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // position
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // normal
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT }, // texcoord
	}, fmt);

	r_vertex_buffer_create(vbuf, 8192 * sizeof(StaticModelVertex), 3, fmt);

	StaticModelVertex quad[] = {
		{ { -0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 0, 0 } },
		{ { -0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 0, 1 } },
		{ {  0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 1, 1 } },
		{ {  0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 1, 0 } },
	};

	r_vertex_buffer_append(vbuf, sizeof(quad), quad);
	r_vertex_buffer(vbuf);
}

static void r_reset_sprites_vbuf(VertexBuffer *vbuf) {
	r_vertex_buffer_invalidate(vbuf);
	r_vertex_buffer_append(vbuf, sizeof(R.quad_2d), &R.quad_2d);
}

static void r_init_vbuf_sprites(VertexBuffer *vbuf) {
	size_t sz_vert = sizeof(Quad2DVertex);
	size_t sz_attr = sizeof(Sprite2DAttrs);
	size_t sz_quad = sizeof(R.quad_2d);

	#define QUAD_OFS(attr)     offsetof(Quad2DVertex,  attr)
	#define INSTANCE_OFS(attr) offsetof(Sprite2DAttrs, attr) + sz_quad

	VertexAttribFormat fmt[] = {
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT,            0 }, sz_vert, QUAD_OFS(pos)              },

		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[0]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[1]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[2]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[3]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(rgba)         },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(texrect)      },
		{ { 1, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(custom)       },
		{ { 2, VA_UBYTE, VA_CONVERT_FLOAT_NORMALIZED, 1 }, sz_attr, INSTANCE_OFS(flip)         },
	};

	#undef QUAD_OFS
	#undef INSTANCE_OFS

	r_vertex_buffer_create(vbuf, sizeof(R.quad_2d) + SPRITE_BATCH_CAPACITY * sizeof(Sprite2DAttrs), sizeof(fmt)/sizeof(*fmt), fmt);
	r_reset_sprites_vbuf(vbuf);
}

void r_init(void) {
	_r_init();
	_r_mat_init();
	r_init_vbuf_models(&R.vbufs.models);
	r_init_vbuf_sprites(&R.vbufs.sprites);

	// XXX: is it the right place for this?
	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"sprite_default",
		"texture_post_load",
	NULL);
}

void r_shutdown(void) {
	r_vertex_buffer_destroy(&R.vbufs.models);
	r_vertex_buffer_destroy(&R.vbufs.sprites);
	_r_shutdown();
}

void r_flush_sprites(void) {
	if(R.sprite_batch.num_pending == 0) {
		return;
	}

	uint pending = R.sprite_batch.num_pending;

	// needs to be done early to thwart recursive calls
	R.sprite_batch.num_pending = 0;

	// log_warn("flush! %u", pending);

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	Texture *tex_saved = r_texture_current(0);
	ShaderProgram *prog_saved = r_shader_current();
	RenderTarget *target_saved = r_target_current();
	BlendMode blend_saved = r_blend_current();
	bool cap_deptp_test_saved = r_capability_current(RCAP_DEPTH_TEST);
	bool cap_depth_write_saved = r_capability_current(RCAP_DEPTH_WRITE);
	bool cap_cull_saved = r_capability_current(RCAP_CULL_FACE);
	DepthTestFunc depth_func_saved = r_depth_func_current();
	CullFaceMode cull_mode_saved = r_cull_current();

	r_vertex_buffer(&R.vbufs.sprites);
	r_texture_ptr(0, R.sprite_batch.tex);
	r_shader_ptr(R.sprite_batch.shader);
	r_target(R.sprite_batch.render_target);
	r_blend(R.sprite_batch.blend);
	r_capability(RCAP_DEPTH_TEST, R.sprite_batch.depth_test_enabled);
	r_capability(RCAP_DEPTH_WRITE, R.sprite_batch.depth_write_enabled);
	r_capability(RCAP_CULL_FACE, R.sprite_batch.cull_enabled);
	r_depth_func(R.sprite_batch.depth_func);
	r_cull(R.sprite_batch.cull_mode);

	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, pending);
	r_reset_sprites_vbuf(&R.vbufs.sprites);

	r_vertex_buffer(vbuf_saved);
	r_texture_ptr(0, tex_saved);
	r_shader_ptr(prog_saved);
	r_target(target_saved);
	r_blend(blend_saved);
	r_capability(RCAP_DEPTH_TEST, cap_deptp_test_saved);
	r_capability(RCAP_DEPTH_WRITE, cap_depth_write_saved);
	r_capability(RCAP_CULL_FACE, cap_cull_saved);
	r_depth_func(depth_func_saved);
	r_cull(cull_mode_saved);
}

static void spritebatch_add(Sprite *spr, const SpriteParams *params, VertexBuffer *vbuf) {
	Sprite2DAttrs alignas(32) attribs;
	r_mat_current(MM_MODELVIEW, attribs.transform);

	float scale_x = params->scale.x ? params->scale.x : 1;
	float scale_y = params->scale.y ? params->scale.y : scale_x;

	if(params->pos.x || params->pos.y) {
		glm_translate(attribs.transform, (vec3) { params->pos.x, params->pos.y });
	}

	if(params->rotation.angle) {
		float *rvec = (float*)params->rotation.vector;

		if(rvec[0] == 0 && rvec[1] == 0 && rvec[2] == 0) {
			glm_rotate(attribs.transform, params->rotation.angle, (vec3) { 0, 0, 1 });
		} else {
			glm_rotate(attribs.transform, params->rotation.angle, rvec);
		}
	}

	glm_scale(attribs.transform, (vec3) { scale_x * spr->w, scale_y * spr->h });

	if(params->color == 0) {
		// XXX: should we use r_color_current here?
		parse_color_array(rgba(1, 1, 1, 1), attribs.rgba);
	} else {
		parse_color_array(params->color, attribs.rgba);
	}

	attribs.texrect[0] = spr->tex_area.x / spr->tex->w;
	attribs.texrect[1] = spr->tex_area.y / spr->tex->h;
	attribs.texrect[2] = spr->tex_area.w / spr->tex->w;
	attribs.texrect[3] = spr->tex_area.h / spr->tex->h;
	attribs.flip[0] = params->flip.x ? 0xFF : 0x0;
	attribs.flip[1] = params->flip.y ? 0xFF : 0x0;
	attribs.custom = params->custom;

	r_vertex_buffer_append(vbuf, sizeof(attribs), &attribs);

	if(vbuf->size - vbuf->offset < sizeof(attribs)) {
		log_warn("Vertex buffer exhausted (%zu needed for next sprite, %u remaining), flush forced", sizeof(attribs), vbuf->size - vbuf->offset);
		r_flush_sprites();
	}
}

void r_draw_sprite(const SpriteParams *params) {
	assert(!(params->shader && params->shader_ptr));
	assert(!(params->sprite && params->sprite_ptr));

	Sprite *spr = params->sprite_ptr;

	if(spr == NULL) {
		assert(params->sprite != NULL);
		spr = get_sprite(params->sprite);
	}

	if(spr->tex != R.sprite_batch.tex) {
		r_flush_sprites();
		R.sprite_batch.tex = spr->tex;
	}

	ShaderProgram *prog = params->shader_ptr;

	if(prog == NULL) {
		if(params->shader == NULL) {
			prog = r_shader_current();
		} else {
			prog = r_shader_get(params->shader);
		}
	}

	assert(prog != NULL);

	if(prog != R.sprite_batch.shader) {
		r_flush_sprites();
		R.sprite_batch.shader = prog;
	}

	RenderTarget *target = r_target_current();

	if(target != R.sprite_batch.render_target) {
		r_flush_sprites();
		R.sprite_batch.render_target = target;
	}

	BlendMode blend = params->blend;

	if(blend == 0) {
		blend = r_blend_current();
	}

	if(blend != R.sprite_batch.blend) {
		r_flush_sprites();
		R.sprite_batch.blend = blend;
	}

	bool depth_test_enabled = r_capability_current(RCAP_DEPTH_TEST);
	bool depth_write_enabled = r_capability_current(RCAP_DEPTH_WRITE);
	bool cull_enabled = r_capability_current(RCAP_CULL_FACE);
	DepthTestFunc depth_func = r_depth_func_current();
	CullFaceMode cull_mode = r_cull_current();

	if(R.sprite_batch.depth_test_enabled != depth_test_enabled) {
		r_flush_sprites();
		R.sprite_batch.depth_test_enabled = depth_test_enabled;
	}

	if(R.sprite_batch.depth_write_enabled != depth_write_enabled) {
		r_flush_sprites();
		R.sprite_batch.depth_write_enabled = depth_write_enabled;
	}

	if(R.sprite_batch.cull_enabled != cull_enabled) {
		r_flush_sprites();
		R.sprite_batch.cull_enabled = cull_enabled;
	}

	if(R.sprite_batch.depth_func != depth_func) {
		r_flush_sprites();
		R.sprite_batch.depth_func = depth_func;
	}

	if(R.sprite_batch.cull_mode != cull_mode) {
		r_flush_sprites();
		R.sprite_batch.cull_mode = cull_mode;
	}

	R.sprite_batch.num_pending++;
	spritebatch_add(spr, params, &R.vbufs.sprites);
}

VertexBuffer* r_vertex_buffer_static_models(void) {
	return &R.vbufs.models;
}

void r_draw_quad(void) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.vbufs.models);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, 0);
	r_vertex_buffer(vbuf_saved);
}

void r_draw_quad_instanced(uint instances) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.vbufs.models);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, instances);
	r_vertex_buffer(vbuf_saved);
}

void r_draw_model_ptr(Model *model) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	r_vertex_buffer(&R.vbufs.models);
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
