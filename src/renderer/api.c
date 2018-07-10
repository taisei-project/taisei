/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "api.h"
#include "common/backend.h"
#include "common/matstack.h"
#include "common/sprite_batch.h"
#include "common/models.h"
#include "common/state.h"
#include "util/glm.h"

#define B _r_backend.funcs

static struct {
	struct {
		ShaderProgram *standard;
		ShaderProgram *standardnotex;
	} progs;
} R;

void r_init(void) {
	_r_backend_init();
}

void r_post_init(void) {
	_r_state_init();
	B.post_init();
	_r_mat_init();
	_r_models_init();
	_r_sprite_batch_init();

	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"sprite_default",
		"texture_post_load",
		"standard",
		"standardnotex",
	NULL);

	R.progs.standard = r_shader_get("standard");
	R.progs.standardnotex = r_shader_get("standardnotex");

	r_shader_ptr(R.progs.standard);
	r_color4(1, 1, 1, 1);

	log_info("Rendering subsystem initialized (%s)", _r_backend.name);
}

void r_shutdown(void) {
	_r_state_shutdown();
	_r_sprite_batch_shutdown();
	_r_models_shutdown();
	B.shutdown();
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

bool r_supports(RendererFeature feature) {
	return B.supports(feature);
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

Color r_color_current(void) {
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

void r_shader_ptr(ShaderProgram *prog) {
	_r_state_touch_shader();
	B.shader(prog);
}

ShaderProgram* r_shader_current(void) {
	return B.shader_current();
}

Uniform* r_shader_uniform(ShaderProgram *prog, const char *uniform_name) {
	return B.shader_uniform(prog, uniform_name);
}

void r_uniform_ptr(Uniform *uniform, uint count, const void *data) {
	_r_state_touch_uniform(uniform);
	B.uniform(uniform, count, data);
}

UniformType r_uniform_type(Uniform *uniform) {
	return B.uniform_type(uniform);
}

void r_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances, uint base_instance) {
	B.draw(prim, first, count, indices, instances, base_instance);
}

void r_texture_create(Texture *tex, const TextureParams *params) {
	B.texture_create(tex, params);
}

void r_texture_get_params(Texture *tex, TextureParams *params) {
	B.texture_get_params(tex, params);
}

void r_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	B.texture_set_filter(tex, fmin, fmag);
}

void r_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	B.texture_set_wrap(tex, ws, wt);
}

void r_texture_fill(Texture *tex, uint mipmap, void *image_data) {
	B.texture_fill(tex, mipmap, image_data);
}

void r_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data) {
	B.texture_fill_region(tex, mipmap, x, y, w, h, image_data);
}

void r_texture_invalidate(Texture *tex) {
	B.texture_invalidate(tex);
}

void r_texture_destroy(Texture *tex) {
	B.texture_destroy(tex);
}

void r_texture_ptr(uint unit, Texture *tex) {
	_r_state_touch_texunit(unit);
	B.texture(unit, tex);
}

Texture* r_texture_current(uint unit) {
	return B.texture_current(unit);
}

void r_framebuffer_create(Framebuffer *fb) {
	B.framebuffer_create(fb);
}

void r_framebuffer_attach(Framebuffer *fb, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	B.framebuffer_attach(fb, tex, mipmap, attachment);
}

Texture* r_framebuffer_get_attachment(Framebuffer *fb, FramebufferAttachment attachment) {
	return B.framebuffer_get_attachment(fb, attachment);
}

uint r_framebuffer_get_attachment_mipmap(Framebuffer *fb, FramebufferAttachment attachment) {
	return B.framebuffer_get_attachment_mipmap(fb, attachment);
}

void r_framebuffer_viewport(Framebuffer *fb, int x, int y, int w, int h) {
	r_framebuffer_viewport_rect(fb, (IntRect) { x, y, w, h });
}

void r_framebuffer_viewport_rect(Framebuffer *fb, IntRect viewport) {
	assert(viewport.h > 0);
	assert(viewport.w > 0);
	B.framebuffer_viewport(fb, viewport);
}

void r_framebuffer_viewport_current(Framebuffer *fb, IntRect *viewport) {
	B.framebuffer_viewport_current(fb, viewport);
}

void r_framebuffer_destroy(Framebuffer *fb) {
	B.framebuffer_destroy(fb);
}

void r_framebuffer(Framebuffer *fb) {
	_r_state_touch_framebuffer();
	B.framebuffer(fb);
}

Framebuffer * r_framebuffer_current(void) {
	return B.framebuffer_current();
}

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data) {
	B.vertex_buffer_create(vbuf, capacity, data);
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	B.vertex_buffer_destroy(vbuf);
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	B.vertex_buffer_invalidate(vbuf);
}

void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) {
	B.vertex_buffer_write(vbuf, offset, data_size, data);
}

void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	B.vertex_buffer_append(vbuf, data_size, data);
}

void r_vertex_array_create(VertexArray *varr) {
	B.vertex_array_create(varr);
}

void r_vertex_array_destroy(VertexArray *varr) {
	B.vertex_array_destroy(varr);
}

void r_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	B.vertex_array_attach_buffer(varr, vbuf, attachment);
}

VertexBuffer* r_vertex_array_get_attachment(VertexArray *varr, uint attachment) {
	return B.vertex_array_get_attachment(varr, attachment);
}

void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	B.vertex_array_layout(varr, nattribs, attribs);
}

void r_vertex_array(VertexArray *varr) {
	_r_state_touch_vertex_array();
	B.vertex_array(varr);
}

VertexArray* r_vertex_array_current(void) {
	return B.vertex_array_current();
}

void r_clear(ClearBufferFlags flags) {
	B.clear(flags);
}

void r_clear_color4(float r, float g, float b, float a) {
	_r_state_touch_clear_color();
	B.clear_color4(r, g, b, a);
}

Color r_clear_color_current(void) {
	return B.clear_color_current();
}

void r_vsync(VsyncMode mode) {
	_r_state_touch_vsync();
	B.vsync(mode);
}

VsyncMode r_vsync_current(void) {
	return B.vsync_current();
}

void r_swap(SDL_Window *window) {
	_r_sprite_batch_end_frame();
	B.swap(window);
}

uint8_t* r_screenshot(uint *out_width, uint *out_height) {
	return B.screenshot(out_width, out_height);
}
