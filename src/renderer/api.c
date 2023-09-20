/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "api.h"
#include "common/backend.h"
#include "common/matstack.h"
#include "common/sprite_batch.h"
#include "common/models.h"
#include "common/state.h"
#include "util/glm.h"
#include "util/graphics.h"
#include "resource/resource.h"
#include "resource/texture.h"
#include "resource/sprite.h"
#include "coroutine.h"

#define B _r_backend.funcs

static struct {
	ResourceGroup rg;
	struct {
		ShaderProgram *standard;
		ShaderProgram *standardnotex;
	} progs;
} R;

void r_init(void) {
	_r_backend_init();
}

typedef struct BlurInfo {
	const char *progname;
	int kernel_size;
	float sigma;
} BlurInfo;

void r_post_init(void) {
	_r_state_init();
	B.post_init();
	_r_mat_init();
	_r_models_init();
	_r_sprite_batch_init();

	res_group_init(&R.rg);

	res_group_preload(&R.rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"sprite_default",
		"texture_post_load",
		"standard",
		"standardnotex",
	NULL);

#if DEBUG
	res_group_preload(&R.rg, RES_FONT, RESF_DEFAULT,
		"monotiny",
	NULL);
#endif

	R.progs.standard = res_shader("standard");
	R.progs.standardnotex = res_shader("standardnotex");

	r_shader_ptr(R.progs.standard);
	r_color4(1, 1, 1, 1);
	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_enable(RCAP_CULL_FACE);
	r_depth_func(DEPTH_LEQUAL);
	r_cull(CULL_BACK);
	r_blend(BLEND_PREMUL_ALPHA);
	r_framebuffer_clear(NULL, BUFFER_ALL, RGBA(0, 0, 0, 1), 1);

	log_info("Rendering subsystem initialized (%s)", _r_backend.name);
}

void r_release_resources(void) {
	res_group_release(&R.rg);
}

void r_shutdown(void) {
	_r_state_shutdown();
	_r_sprite_batch_shutdown();
	_r_models_shutdown();
	B.shutdown();
}

const char *r_backend_name(void) {
	return _r_backend.name;
}

void r_shader_standard(void) {
	r_shader_ptr(R.progs.standard);
}

void r_shader_standard_notex(void) {
	r_shader_ptr(R.progs.standardnotex);
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
		[UNIFORM_FLOAT]        = {  1, sizeof(float) },
		[UNIFORM_VEC2]         = {  2, sizeof(float) },
		[UNIFORM_VEC3]         = {  3, sizeof(float) },
		[UNIFORM_VEC4]         = {  4, sizeof(float) },
		[UNIFORM_INT]          = {  1, sizeof(int)   },
		[UNIFORM_IVEC2]        = {  2, sizeof(int)   },
		[UNIFORM_IVEC3]        = {  3, sizeof(int)   },
		[UNIFORM_IVEC4]        = {  4, sizeof(int)   },
		[UNIFORM_SAMPLER_2D]   = {  1, sizeof(void*) },
		[UNIFORM_SAMPLER_CUBE] = {  1, sizeof(void*) },
		[UNIFORM_MAT3]         = {  9, sizeof(float) },
		[UNIFORM_MAT4]         = { 16, sizeof(float) },
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

VertexAttribFormat* r_vertex_attrib_format_interleaved(
	size_t nattribs,
	VertexAttribSpec specs[nattribs],
	VertexAttribFormat formats[nattribs],
	uint attachment
) {
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
		formats[i].attachment = attachment;
	}

	return formats + nattribs;
}

// TODO: Maybe implement the state tracker/redundant call elimination here somehow
// instead of burdening the backend with it.
//
// For now though, enjoy this degeneracy since the internal backend interface is
// nearly identical to the public API currently.
//
// A more realisic TODO would be to put assertions/argument validation here.

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	return B.create_window(title, x, y, w, h, flags);
}

r_feature_bits_t r_features(void) {
	return B.features();
}

r_capability_bits_t r_capabilities_current(void) {
	return B.capabilities_current();
}

void r_capabilities(r_capability_bits_t newcaps) {
	r_capability_bits_t caps = B.capabilities_current();

	if(caps != newcaps) {
		_r_state_touch_capabilities();
		B.capabilities(newcaps);
	}
}

void r_capability(RendererCapability cap, bool value) {
	r_capability_bits_t caps = B.capabilities_current(), newcaps;

	if(value) {
		newcaps = caps | r_capability_bit(cap);
	} else {
		newcaps = caps & ~r_capability_bit(cap);
	}

	if(caps != newcaps) {
		_r_state_touch_capabilities();
		B.capabilities(newcaps);
	}
}

bool r_capability_current(RendererCapability cap) {
	return B.capabilities_current() & r_capability_bit(cap);
}

void r_color4(float r, float g, float b, float a) {
	_r_state_touch_color();
	B.color4(r, g, b, a);
}

const Color* r_color_current(void) {
	return B.color_current();
}

void r_blend(BlendMode mode) {
	_r_state_touch_blend_mode();
	B.blend(mode);
}

BlendMode r_blend_current(void) {
	return B.blend_current();
}

void r_cull(CullFaceMode mode) {
	_r_state_touch_cull_mode();
	B.cull(mode);
}

CullFaceMode r_cull_current(void) {
	return B.cull_current();
}

void r_depth_func(DepthTestFunc func) {
	_r_state_touch_depth_func();
	B.depth_func(func);
}

DepthTestFunc r_depth_func_current(void) {
	return B.depth_func_current();
}

bool r_shader_language_supported(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative) {
	return B.shader_language_supported(lang, out_alternative);
}

ShaderObject* r_shader_object_compile(ShaderSource *source) {
	return B.shader_object_compile(source);
}

void r_shader_object_destroy(ShaderObject *shobj) {
	B.shader_object_destroy(shobj);
}

void r_shader_object_set_debug_label(ShaderObject *shobj, const char *label) {
	B.shader_object_set_debug_label(shobj, label);
}

const char* r_shader_object_get_debug_label(ShaderObject *shobj) {
	return B.shader_object_get_debug_label(shobj);
}

bool r_shader_object_transfer(ShaderObject *dst, ShaderObject *src) {
	return B.shader_object_transfer(dst, src);
}

ShaderProgram* r_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) {
	return B.shader_program_link(num_objects, shobjs);
}

void r_shader_program_destroy(ShaderProgram *prog) {
	B.shader_program_destroy(prog);
}

void r_shader_program_set_debug_label(ShaderProgram *prog, const char *label) {
	B.shader_program_set_debug_label(prog, label);
}

const char* r_shader_program_get_debug_label(ShaderProgram *prog) {
	return B.shader_program_get_debug_label(prog);
}

bool r_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src) {
	return B.shader_program_transfer(dst, src);
}

void r_shader_ptr(ShaderProgram *prog) {
	_r_state_touch_shader();
	B.shader(prog);
}

ShaderProgram* r_shader_current(void) {
	return B.shader_current();
}

Uniform* _r_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash) {
	return B.shader_uniform(prog, uniform_name, uniform_name_hash);
}

UniformType r_uniform_type(Uniform *uniform) {
	return B.uniform_type(uniform);
}

void r_draw(VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance) {
	B.draw(varr, prim, firstvert, count, instances, base_instance);
}

void r_draw_indexed(VertexArray* varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance) {
	B.draw_indexed(varr, prim, firstidx, count, instances, base_instance);
}

Texture *r_texture_create(const TextureParams *params) {
	assert(r_texture_type_query(params->type, params->flags, 0, 0, NULL));
	return B.texture_create(params);
}

void r_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) {
	B.texture_get_size(tex, mipmap, width, height);
}

uint r_texture_get_width(Texture *tex, uint mipmap) {
	uint w;
	B.texture_get_size(tex, mipmap, &w, NULL);
	return w;
}

uint r_texture_get_height(Texture *tex, uint mipmap) {
	uint h;
	B.texture_get_size(tex, mipmap, NULL, &h);
	return h;
}

void r_texture_get_params(Texture *tex, TextureParams *params) {
	B.texture_get_params(tex, params);
}

const char* r_texture_get_debug_label(Texture *tex) {
	return B.texture_get_debug_label(tex);
}

void r_texture_set_debug_label(Texture *tex, const char *label) {
	B.texture_set_debug_label(tex, label);
}

void r_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	B.texture_set_filter(tex, fmin, fmag);
}

void r_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	B.texture_set_wrap(tex, ws, wt);
}

void r_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image_data) {
	B.texture_fill(tex, mipmap, layer, image_data);
}

void r_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image_data) {
	B.texture_fill_region(tex, mipmap, layer, x, y, image_data);
}

bool r_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst) {
	return B.texture_dump(tex, mipmap, layer, dst);
}

void r_texture_invalidate(Texture *tex) {
	B.texture_invalidate(tex);
}

void r_texture_clear(Texture *tex, const Color *clr) {
	B.texture_clear(tex, clr);
}

void r_texture_destroy(Texture *tex) {
	B.texture_destroy(tex);
}

bool r_texture_transfer(Texture *dst, Texture *src) {
	return B.texture_transfer(dst, src);
}

bool r_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result) {
	return B.texture_type_query(type, flags, pxfmt, pxorigin, result);
}

const char *r_texture_type_name(TextureType type) {
	switch(type) {
		#define HANDLE_TYPE(type, ...) \
		case TEX_TYPE_##type: return #type;
		TEX_TYPES(HANDLE_TYPE,)
		#undef HANDLE_TYPE
		default: UNREACHABLE;
	}
}

TextureType r_texture_type_from_pixmap_format(PixmapFormat fmt) {
	if(pixmap_format_is_compressed(fmt)) {
		TextureType t = pixmap_format_compression(fmt) | TEX_TYPE_COMPRESSED_BIT;
		assert(r_texture_type_name(t) != NULL);
		return t;
	}

	PixmapLayout layout = pixmap_format_layout(fmt);
	uint depth = pixmap_format_depth(fmt);
	bool is_float = pixmap_format_is_float(fmt);

	if(is_float) {
		switch(depth) {
			case 16: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_16_FLOAT;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_16_FLOAT;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_16_FLOAT;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_16_FLOAT;
				default: UNREACHABLE;
			}

			case 32: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_32_FLOAT;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_32_FLOAT;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_32_FLOAT;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_32_FLOAT;
				default: UNREACHABLE;
			}

			default: UNREACHABLE;
		}
	} else {
		switch(depth) {
			case 8: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_8;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_8;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_8;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_8;
				default: UNREACHABLE;
			}

			case 16: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_16;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_16;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_16;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_16;
				default: UNREACHABLE;
			}

			default: UNREACHABLE;
		}
	}
}

uint r_texture_util_max_num_miplevels(uint width, uint height) {
	uint dim = umax(width, height);
	return dim > 0 ? 1 + floor(log2(dim)) : 0;  // TODO replace with integer log2
}

Framebuffer* r_framebuffer_create(void) {
	return B.framebuffer_create();
}

const char* r_framebuffer_get_debug_label(Framebuffer *fb) {
	return B.framebuffer_get_debug_label(fb);
}

void r_framebuffer_set_debug_label(Framebuffer *fb, const char *label) {
	B.framebuffer_set_debug_label(fb, label);
}

void r_framebuffer_attach(Framebuffer *fb, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	B.framebuffer_attach(fb, tex, mipmap, attachment);
}

Texture *r_framebuffer_get_attachment(Framebuffer *fb, FramebufferAttachment attachment) {
	return B.framebuffer_query_attachment(fb, attachment).texture;
}

uint r_framebuffer_get_attachment_mipmap(Framebuffer *fb, FramebufferAttachment attachment) {
	return B.framebuffer_query_attachment(fb, attachment).miplevel;
}

void r_framebuffer_set_output_attachment(Framebuffer *fb, uint output, FramebufferAttachment attachment) {
	FramebufferAttachment temp[FRAMEBUFFER_MAX_OUTPUTS];
	assert(output < FRAMEBUFFER_MAX_OUTPUTS);
	temp[output] = attachment;
	B.framebuffer_outputs(fb, temp, 1 << output);
}

FramebufferAttachment r_framebuffer_get_output_attachment(Framebuffer *fb, uint output) {
	FramebufferAttachment temp[FRAMEBUFFER_MAX_OUTPUTS];
	assert(output < FRAMEBUFFER_MAX_OUTPUTS);
	B.framebuffer_outputs(fb, temp, 0x00);
	return temp[output];
}

void r_framebuffer_set_output_attachments(Framebuffer *fb, const FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS]) {
	B.framebuffer_outputs(fb, (FramebufferAttachment*)config, 0xff);
}

void r_framebuffer_get_output_attachments(Framebuffer *fb, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS]) {
	B.framebuffer_outputs(fb, config, 0x00);
}

void r_framebuffer_clear(Framebuffer *fb, BufferKindFlags flags, const Color *colorval, float depthval) {
	B.framebuffer_clear(fb, flags, colorval, depthval);
}

void r_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) {
	B.framebuffer_copy(dst, src, flags);
}

void r_framebuffer_viewport(Framebuffer *fb, float x, float y, float w, float h) {
	r_framebuffer_viewport_rect(fb, (FloatRect) { x, y, w, h });
}

void r_framebuffer_viewport_rect(Framebuffer *fb, FloatRect viewport) {
	assert(viewport.h > 0);
	assert(viewport.w > 0);
	B.framebuffer_viewport(fb, viewport);
}

void r_framebuffer_viewport_current(Framebuffer *fb, FloatRect *viewport) {
	B.framebuffer_viewport_current(fb, viewport);
}

void r_framebuffer_destroy(Framebuffer *fb) {
	B.framebuffer_destroy(fb);
}

void r_framebuffer(Framebuffer *fb) {
	_r_state_touch_framebuffer();
	B.framebuffer(fb);
}

IntExtent r_framebuffer_get_size(Framebuffer *fb) {
	return B.framebuffer_get_size(fb);
}

Framebuffer* r_framebuffer_current(void) {
	return B.framebuffer_current();
}

VertexBuffer* r_vertex_buffer_create(size_t capacity, void *data) {
	return B.vertex_buffer_create(capacity, data);
}

const char* r_vertex_buffer_get_debug_label(VertexBuffer *vbuf) {
	return B.vertex_buffer_get_debug_label(vbuf);
}

void r_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char* label) {
	B.vertex_buffer_set_debug_label(vbuf, label);
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	B.vertex_buffer_destroy(vbuf);
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	B.vertex_buffer_invalidate(vbuf);
}

SDL_RWops* r_vertex_buffer_get_stream(VertexBuffer *vbuf) {
	return B.vertex_buffer_get_stream(vbuf);
}

IndexBuffer* r_index_buffer_create(uint index_size, size_t max_elements) {
	return B.index_buffer_create(index_size, max_elements);
}

size_t r_index_buffer_get_capacity(IndexBuffer *ibuf) {
	return B.index_buffer_get_capacity(ibuf);
}

uint r_index_buffer_get_index_size(IndexBuffer *ibuf) {
	return B.index_buffer_get_index_size(ibuf);
}

const char* r_index_buffer_get_debug_label(IndexBuffer *ibuf) {
	return B.index_buffer_get_debug_label(ibuf);
}

void r_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) {
	B.index_buffer_set_debug_label(ibuf, label);
}

void r_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) {
	B.index_buffer_set_offset(ibuf, offset);
}

size_t r_index_buffer_get_offset(IndexBuffer *ibuf) {
	return B.index_buffer_get_offset(ibuf);
}

#define MAX_INDEX_VAL(isize) ((1ull << ((isize) * 8)) - 1)
#define INDEX_WRITE_LOOP(itype, isize) \
	itype data[num_elements]; \
	attr_unused const uintmax_t max_val = MAX_INDEX_VAL(isize); \
	for(size_t i = 0; i < num_elements; ++i) { \
		assert((uintmax_t)indices[i] + (uintmax_t)index_ofs <= max_val); \
		data[i] = indices[i] + index_ofs; \
	} \
	B.index_buffer_add_indices(ibuf, index_size * num_elements, data);

void r_index_buffer_add_indices_u16(
	IndexBuffer *ibuf, uint16_t index_ofs, size_t num_elements, uint16_t indices[num_elements]
) {
	uint index_size = r_index_buffer_get_index_size(ibuf);

	if(index_size == 2) {
		if(index_ofs == 0) {
			B.index_buffer_add_indices(ibuf, 2 * num_elements, indices);
			return;
		}

		INDEX_WRITE_LOOP(uint16_t, 2);
	} else {
		assert(index_size == 4);
		INDEX_WRITE_LOOP(uint32_t, 4);
	}
}

void r_index_buffer_add_indices_u32(
	IndexBuffer *ibuf, uint32_t index_ofs, size_t num_elements, uint32_t indices[num_elements]
) {
	uint index_size = r_index_buffer_get_index_size(ibuf);

	if(index_size == 4) {
		if(index_ofs == 0) {
			B.index_buffer_add_indices(ibuf, 4 * num_elements, indices);
			return;
		}

		INDEX_WRITE_LOOP(uint32_t, 4);
	} else {
		assert(index_size == 2);
		INDEX_WRITE_LOOP(uint16_t, 2);
	}
}

void r_index_buffer_invalidate(IndexBuffer *ibuf) {
	B.index_buffer_invalidate(ibuf);
}

void r_index_buffer_destroy(IndexBuffer *ibuf) {
	B.index_buffer_destroy(ibuf);
}

VertexArray* r_vertex_array_create(void) {
	return B.vertex_array_create();
}

const char* r_vertex_array_get_debug_label(VertexArray *varr) {
	return B.vertex_array_get_debug_label(varr);
}

void r_vertex_array_set_debug_label(VertexArray *varr, const char* label) {
	B.vertex_array_set_debug_label(varr, label);
}

void r_vertex_array_destroy(VertexArray *varr) {
	B.vertex_array_destroy(varr);
}

void r_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	B.vertex_array_attach_vertex_buffer(varr, vbuf, attachment);
}

VertexBuffer* r_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment) {
	return B.vertex_array_get_vertex_attachment(varr, attachment);
}

void r_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf) {
	B.vertex_array_attach_index_buffer(varr, ibuf);
}

IndexBuffer* r_vertex_array_get_index_attachment(VertexArray *varr) {
	return B.vertex_array_get_index_attachment(varr);
}

void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	B.vertex_array_layout(varr, nattribs, attribs);
}

void r_scissor(int x, int y, int w, int h) {
	_r_state_touch_scissor();
	B.scissor((IntRect) { x, y, w, h });
}

void r_scissor_rect(IntRect scissor) {
	B.scissor(scissor);
}

void r_scissor_current(IntRect *scissor) {
	B.scissor_current(scissor);
}

void r_vsync(VsyncMode mode) {
	B.vsync(mode);
}

VsyncMode r_vsync_current(void) {
	return B.vsync_current();
}

void r_swap(SDL_Window *window) {
	coroutines_draw_stats();
	_r_sprite_batch_end_frame();
	B.swap(window);
}

bool r_screenshot(Pixmap *out) {
	return B.screenshot(out);
}

// uniforms garbage; hope your compiler is smart enough to inline most of this

// TODO: verify sampler-to-texture type consistency?

#define ASSERT_UTYPE(uniform, type) do { \
	if(uniform) { \
		assert(r_uniform_type(uniform) == type); \
	} \
} while(0)

#define ASSERT_UTYPE_SAMPLER(uniform) do { \
	if(uniform) { \
		attr_unused UniformType _utype = r_uniform_type(uniform); \
		assert(UNIFORM_TYPE_IS_SAMPLER(_utype)); \
	} \
} while(0)

void r_uniform_ptr_unsafe(Uniform *uniform, uint offset, uint count, void *data) {
	if(uniform) B.uniform(uniform, offset, count, data);
}

void _r_uniform_ptr_float(Uniform *uniform, float value) {
	ASSERT_UTYPE(uniform, UNIFORM_FLOAT);
	if(uniform) B.uniform(uniform, 0, 1, &value);
}

void _r_uniform_float(const char *uniform, float value) {
	_r_uniform_ptr_float(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_float_array(Uniform *uniform, uint offset, uint count, float elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_FLOAT);
	if(uniform && count) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_float_array(const char *uniform, uint offset, uint count, float elements[count]) {
	_r_uniform_ptr_float_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_vec2(Uniform *uniform, float x, float y) {
	_r_uniform_ptr_vec2_vec(uniform, (vec2_noalign) { x, y });
}

void _r_uniform_vec2(const char *uniform, float x, float y) {
	_r_uniform_vec2_vec(uniform, (vec2_noalign) { x, y });
}

void _r_uniform_ptr_vec2_vec(Uniform *uniform, vec2_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC2);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_vec2_vec(const char *uniform, vec2_noalign value) {
	_r_uniform_ptr_vec2_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_vec2_complex(Uniform *uniform, cmplx value) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC2);
	if(uniform) B.uniform(uniform, 0, 1, (vec2_noalign) { re(value), im(value) });
}

void _r_uniform_vec2_complex(const char *uniform, cmplx value) {
	_r_uniform_ptr_vec2_complex(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_vec2_array(Uniform *uniform, uint offset, uint count, vec2_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC2);
	if(uniform && count) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_vec2_array(const char *uniform, uint offset, uint count, vec2_noalign elements[count]) {
	_r_uniform_ptr_vec2_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_vec2_array_complex(Uniform *uniform, uint offset, uint count, cmplx elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC2);
	if(uniform && count) {
		float arr[2 * count];
		cmplx *eptr = elements;
		float *aptr = arr, *aend = arr + sizeof(arr)/sizeof(*arr);

		do {
			*aptr++ = re(*eptr);
			*aptr++ = im(*eptr++);
		} while(aptr < aend);

		B.uniform(uniform, offset, count, arr);
	}
}

void _r_uniform_vec2_array_complex(const char *uniform, uint offset, uint count, cmplx elements[count]) {
	_r_uniform_ptr_vec2_array_complex(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_vec3(Uniform *uniform, float x, float y, float z) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC3);
	if(uniform) B.uniform(uniform, 0, 1, (vec3_noalign) { x, y, z });
}

void _r_uniform_vec3(const char *uniform, float x, float y, float z) {
	_r_uniform_ptr_vec3(r_shader_current_uniform(uniform), x, y, z);
}

void _r_uniform_ptr_vec3_vec(Uniform *uniform, vec3_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC3);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_vec3_vec(const char *uniform, vec3_noalign value) {
	_r_uniform_ptr_vec3_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_vec3_rgb(Uniform *uniform, const Color *rgb) {
	_r_uniform_ptr_vec3(uniform, rgb->r, rgb->g, rgb->b);
}

void _r_uniform_vec3_rgb(const char *uniform, const Color *rgb) {
	_r_uniform_ptr_vec3_rgb(r_shader_current_uniform(uniform), rgb);
}

void _r_uniform_ptr_vec3_array(Uniform *uniform, uint offset, uint count, vec3_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC3);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_vec3_array(const char *uniform, uint offset, uint count, vec3_noalign elements[count]) {
	_r_uniform_ptr_vec3_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_vec4(Uniform *uniform, float x, float y, float z, float w) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC4);
	if(uniform) B.uniform(uniform, 0, 1, (vec4_noalign) { x, y, z, w });
}

void _r_uniform_vec4(const char *uniform, float x, float y, float z, float w) {
	_r_uniform_ptr_vec4(r_shader_current_uniform(uniform), x, y, z, w);
}

void _r_uniform_ptr_vec4_vec(Uniform *uniform, vec4_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC4);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_vec4_vec(const char *uniform, vec4_noalign value) {
	_r_uniform_ptr_vec4_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_vec4_rgba(Uniform *uniform, const Color *rgba) {
	_r_uniform_ptr_vec4(uniform, rgba->r, rgba->g, rgba->b, rgba->a);
}

void _r_uniform_vec4_rgba(const char *uniform, const Color *rgba) {
	_r_uniform_ptr_vec4_rgba(r_shader_current_uniform(uniform), rgba);
}

void _r_uniform_ptr_vec4_array(Uniform *uniform, uint offset, uint count, vec4_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_VEC4);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_vec4_array(const char *uniform, uint offset, uint count, vec4_noalign elements[count]) {
	_r_uniform_ptr_vec4_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_mat3(Uniform *uniform, mat3_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_MAT3);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_mat3(const char *uniform, mat3_noalign value) {
	_r_uniform_ptr_mat3(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_mat3_array(Uniform *uniform, uint offset, uint count, mat3_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_MAT3);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_mat3_array(const char *uniform, uint offset, uint count, mat3_noalign elements[count]) {
	_r_uniform_ptr_mat3_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_mat4(Uniform *uniform, mat4_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_MAT4);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_mat4(const char *uniform, mat4_noalign value) {
	_r_uniform_ptr_mat4(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_mat4_array(Uniform *uniform, uint offset, uint count, mat4_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_MAT4);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_mat4_array(const char *uniform, uint offset, uint count, mat4_noalign elements[count]) {
	_r_uniform_ptr_mat4_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_int(Uniform *uniform, int value) {
	ASSERT_UTYPE(uniform, UNIFORM_INT);
	if(uniform) B.uniform(uniform, 0, 1, &value);
}

void _r_uniform_int(const char *uniform, int value) {
	_r_uniform_ptr_int(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_int_array(Uniform *uniform, uint offset, uint count, int elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_INT);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_int_array(const char *uniform, uint offset, uint count, int elements[count]) {
	_r_uniform_ptr_int_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_ivec2(Uniform *uniform, int x, int y) {
	_r_uniform_ptr_ivec2_vec(uniform, (ivec2_noalign) { x, y });
}

void _r_uniform_ivec2(const char *uniform, int x, int y) {
	_r_uniform_ivec2_vec(uniform, (ivec2_noalign) { x, y });
}

void _r_uniform_ptr_ivec2_vec(Uniform *uniform, ivec2_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC2);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_ivec2_vec(const char *uniform, ivec2_noalign value) {
	_r_uniform_ptr_ivec2_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_ivec2_array(Uniform *uniform, uint offset, uint count, ivec2_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC2);
	if(uniform && count) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_ivec2_array(const char *uniform, uint offset, uint count, ivec2_noalign elements[count]) {
	_r_uniform_ptr_ivec2_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_ivec3(Uniform *uniform, int x, int y, int z) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC3);
	if(uniform) B.uniform(uniform, 0, 1, (ivec3_noalign) { x, y, z });
}

void _r_uniform_ivec3(const char *uniform, int x, int y, int z) {
	_r_uniform_ptr_ivec3(r_shader_current_uniform(uniform), x, y, z);
}

void _r_uniform_ptr_ivec3_vec(Uniform *uniform, ivec3_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC3);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_ivec3_vec(const char *uniform, ivec3_noalign value) {
	_r_uniform_ptr_ivec3_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_ivec3_array(Uniform *uniform, uint offset, uint count, ivec3_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC3);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_ivec3_array(const char *uniform, uint offset, uint count, ivec3_noalign elements[count]) {
	_r_uniform_ptr_ivec3_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_ivec4(Uniform *uniform, int x, int y, int z, int w) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC4);
	if(uniform) B.uniform(uniform, 0, 1, (ivec4_noalign) { x, y, z, w });
}

void _r_uniform_ivec4(const char *uniform, int x, int y, int z, int w) {
	_r_uniform_ptr_ivec4(r_shader_current_uniform(uniform), x, y, z, w);
}

void _r_uniform_ptr_ivec4_vec(Uniform *uniform, ivec4_noalign value) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC4);
	if(uniform) B.uniform(uniform, 0, 1, value);
}

void _r_uniform_ivec4_vec(const char *uniform, ivec4_noalign value) {
	_r_uniform_ptr_ivec4_vec(r_shader_current_uniform(uniform), value);
}

void _r_uniform_ptr_ivec4_array(Uniform *uniform, uint offset, uint count, ivec4_noalign elements[count]) {
	ASSERT_UTYPE(uniform, UNIFORM_IVEC4);
	if(uniform) B.uniform(uniform, offset, count, elements);
}

void _r_uniform_ivec4_array(const char *uniform, uint offset, uint count, ivec4_noalign elements[count]) {
	_r_uniform_ptr_ivec4_array(r_shader_current_uniform(uniform), offset, count, elements);
}

void _r_uniform_ptr_sampler_ptr(Uniform *uniform, Texture *tex) {
	ASSERT_UTYPE_SAMPLER(uniform);
	if(uniform) B.uniform(uniform, 0, 1, &tex);
}

void _r_uniform_sampler_ptr(const char *uniform, Texture *tex) {
	_r_uniform_ptr_sampler_ptr(r_shader_current_uniform(uniform), tex);
}

void _r_uniform_ptr_sampler(Uniform *uniform, const char *tex) {
	ASSERT_UTYPE_SAMPLER(uniform);
	if(uniform) B.uniform(uniform, 0, 1, (Texture*[]) { res_texture(tex) });
}

void _r_uniform_sampler(const char *uniform, const char *tex) {
	_r_uniform_ptr_sampler(r_shader_current_uniform(uniform), tex);
}

void _r_uniform_ptr_sampler_array_ptr(Uniform *uniform, uint offset, uint count, Texture *values[count]) {
	ASSERT_UTYPE_SAMPLER(uniform);
	if(uniform && count) B.uniform(uniform, offset, count, values);
}

void _r_uniform_sampler_array_ptr(const char *uniform, uint offset, uint count, Texture *values[count]) {
	_r_uniform_ptr_sampler_array_ptr(r_shader_current_uniform(uniform), offset, count, values);
}

void _r_uniform_ptr_sampler_array(Uniform *uniform, uint offset, uint count, const char *values[count]) {
	ASSERT_UTYPE_SAMPLER(uniform);
	if(uniform && count) {
		Texture *arr[count], **aptr = arr, **aend = aptr + count;
		const char **vptr = values;

		do {
			*aptr++ = res_texture(*vptr++);
		} while(aptr < aend);

		B.uniform(uniform, 0, 1, arr);
	}
}

void _r_uniform_sampler_array(const char *uniform, uint offset, uint count, const char *values[count]) {
	_r_uniform_ptr_sampler_array(r_shader_current_uniform(uniform), offset, count, values);
}
