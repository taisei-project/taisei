/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "color.h"
#include "resource/resource.h"
#include "resource/shader_program.h"
#include "resource/texture.h"
#include "resource/model.h"
#include "resource/sprite.h"

typedef enum RendererFeature {
	RFEAT_DRAW_INSTANCED,
	RFEAT_DRAW_INSTANCED_BASE_INSTANCE,
	RFEAT_DEPTH_TEXTURE,
	RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS,

	NUM_RFEATS,
} RendererFeature;

typedef uint_fast8_t r_feature_bits_t;

typedef enum RendererCapability {
	RCAP_DEPTH_TEST,
	RCAP_DEPTH_WRITE,
	RCAP_CULL_FACE,

	NUM_RCAPS
} RendererCapability;

typedef uint_fast8_t r_capability_bits_t;

typedef enum MatrixMode {
	// XXX: Mimicking the gl matrix mode api for easy replacement.
	// But we might just want to add proper functions to modify the non modelview matrices.

	MM_MODELVIEW,
	MM_PROJECTION,
	MM_TEXTURE,
} MatrixMode;

typedef enum TextureType {
	// NOTE: whichever is placed first here is considered the "default" where applicable.
	TEX_TYPE_RGBA,
	TEX_TYPE_RGB,
	TEX_TYPE_RG,
	TEX_TYPE_R,
	TEX_TYPE_DEPTH,
} TextureType;

typedef enum TextureFilterMode {
	// NOTE: whichever is placed first here is considered the "default" where applicable.
	TEX_FILTER_LINEAR,
	TEX_FILTER_LINEAR_MIPMAP_NEAREST,
	TEX_FILTER_LINEAR_MIPMAP_LINEAR,
	TEX_FILTER_NEAREST,
	TEX_FILTER_NEAREST_MIPMAP_NEAREST,
	TEX_FILTER_NEAREST_MIPMAP_LINEAR,
} TextureFilterMode;

typedef enum TextureWrapMode {
	// NOTE: whichever is placed first here is considered the "default" where applicable.
	TEX_WRAP_REPEAT,
	TEX_WRAP_MIRROR,
	TEX_WRAP_CLAMP,
} TextureWrapMode;

typedef enum TextureMipmapMode {
	TEX_MIPMAP_MANUAL,
	TEX_MIPMAP_AUTO,
} TextureMipmapMode;

#define TEX_ANISOTROPY_DEFAULT 1
#define TEX_MIPMAPS_MAX ((uint)(-1))

typedef struct TextureParams {
	uint width;
	uint height;
	TextureType type;

	struct {
		TextureFilterMode mag;
		TextureFilterMode min;
	} filter;

	struct {
		TextureWrapMode s;
		TextureWrapMode t;
	} wrap;

	int anisotropy;
	uint mipmaps;
	TextureMipmapMode mipmap_mode;
	bool stream;
} attr_designated_init TextureParams;

typedef struct TextureImpl TextureImpl;

typedef struct Texture {
	uint w;
	uint h;
	TextureType type;
	TextureImpl *impl;
} Texture;

enum {
	R_MAX_TEXUNITS = 8,
};

typedef enum FramebufferAttachment {
	FRAMEBUFFER_ATTACH_DEPTH,
	FRAMEBUFFER_ATTACH_COLOR0,
	FRAMEBUFFER_ATTACH_COLOR1,
	FRAMEBUFFER_ATTACH_COLOR2,
	FRAMEBUFFER_ATTACH_COLOR3,

	FRAMEBUFFER_MAX_COLOR_ATTACHMENTS = 4,
	FRAMEBUFFER_MAX_ATTACHMENTS = FRAMEBUFFER_ATTACH_COLOR0 + FRAMEBUFFER_MAX_COLOR_ATTACHMENTS,
} FramebufferAttachment;

typedef struct FramebufferImpl FramebufferImpl;

typedef struct Framebuffer {
	FramebufferImpl *impl;
} Framebuffer;

typedef enum Primitive {
	PRIM_POINTS,
	PRIM_LINE_STRIP,
	PRIM_LINE_LOOP,
	PRIM_LINES,
	PRIM_TRIANGLE_STRIP,
	PRIM_TRIANGLE_FAN,
	PRIM_TRIANGLES,
} Primitive;

typedef struct VertexBufferImpl VertexBufferImpl;

typedef struct VertexBuffer {
	uint size;
	uint offset;
	VertexBufferImpl *impl;
} VertexBuffer;

typedef struct VertexArrayImpl VertexArrayImpl;

typedef struct VertexArray {
	VertexArrayImpl *impl;
} VertexArray;

typedef enum VertexAttribType {
	VA_FLOAT,
	VA_BYTE,
	VA_UBYTE,
	VA_SHORT,
	VA_USHORT,
	VA_INT,
	VA_UINT,
} VertexAttribType;

typedef struct VertexAttribTypeInfo {
	size_t size;
	size_t alignment;
} VertexAttribTypeInfo;

typedef enum VertexAttribConversion {
	VA_CONVERT_FLOAT,
	VA_CONVERT_FLOAT_NORMALIZED,
	VA_CONVERT_INT,
} VertexAttribConversion;

typedef struct VertexAttribSpec {
	uint8_t elements;
	VertexAttribType type;
	VertexAttribConversion coversion;
	uint divisor;
} VertexAttribSpec;

typedef struct VertexAttribFormat {
	VertexAttribSpec spec;
	size_t stride;
	size_t offset;
	uint attachment;
} VertexAttribFormat;

typedef struct GenericModelVertex {
	vec3 position;
	vec3 normal;

	struct {
		float s;
		float t;
	} uv;
} GenericModelVertex;

typedef enum UniformType {
	UNIFORM_FLOAT,
	UNIFORM_VEC2,
	UNIFORM_VEC3,
	UNIFORM_VEC4,
	UNIFORM_INT,
	UNIFORM_IVEC2,
	UNIFORM_IVEC3,
	UNIFORM_IVEC4,
	UNIFORM_SAMPLER,
	UNIFORM_MAT3,
	UNIFORM_MAT4,
	UNIFORM_UNKNOWN,
} UniformType;

typedef struct UniformTypeInfo {
	// Refers to vector elements, not array elements.
	uint8_t elements;
	uint8_t element_size;
} UniformTypeInfo;

typedef struct Uniform Uniform;

typedef enum ClearBufferFlags {
	CLEAR_COLOR = (1 << 0),
	CLEAR_DEPTH = (1 << 1),

	CLEAR_ALL = CLEAR_COLOR | CLEAR_DEPTH,
} ClearBufferFlags;

// Blend modes API based on the SDL one.

typedef enum BlendModeComponent {
	BLENDCOMP_COLOR_OP  = 0x00,
	BLENDCOMP_SRC_COLOR = 0x04,
	BLENDCOMP_DST_COLOR = 0x08,
	BLENDCOMP_ALPHA_OP  = 0x10,
	BLENDCOMP_SRC_ALPHA = 0x14,
	BLENDCOMP_DST_ALPHA = 0x18,
} BlendModeComponent;

#define BLENDMODE_COMPOSE(src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op) \
	( \
		((uint32_t) color_op  << BLENDCOMP_COLOR_OP ) | \
		((uint32_t) src_color << BLENDCOMP_SRC_COLOR) | \
		((uint32_t) dst_color << BLENDCOMP_DST_COLOR) | \
		((uint32_t) alpha_op  << BLENDCOMP_ALPHA_OP ) | \
		((uint32_t) src_alpha << BLENDCOMP_SRC_ALPHA) | \
		((uint32_t) dst_alpha << BLENDCOMP_DST_ALPHA)   \
	)

#define BLENDMODE_COMPONENT(mode, comp) \
	(((uint32_t) mode >> (uint32_t) comp) & 0xF)

typedef enum BlendOp {
	BLENDOP_ADD     = 0x1, // dst + src
	BLENDOP_SUB     = 0x2, // dst - src
	BLENDOP_REV_SUB = 0x3, // src - dst
	BLENDOP_MIN     = 0x4, // min(dst, src)
	BLENDOP_MAX     = 0x5, // max(dst, src)
} BlendOp;

typedef enum BlendFactor {
	BLENDFACTOR_ZERO          = 0x1,  //      0,      0,      0,      0
	BLENDFACTOR_ONE           = 0x2,  //      1,      1,      1,      1
	BLENDFACTOR_SRC_COLOR     = 0x3,  //   srcR,   srcG,   srcB,   srcA
	BLENDFACTOR_INV_SRC_COLOR = 0x4,  // 1-srcR, 1-srcG, 1-srcB, 1-srcA
	BLENDFACTOR_SRC_ALPHA     = 0x5,  //   srcA,   srcA,   srcA,   srcA
	BLENDFACTOR_INV_SRC_ALPHA = 0x6,  // 1-srcA, 1-srcA, 1-srcA, 1-srcA
	BLENDFACTOR_DST_COLOR     = 0x7,  //   dstR,   dstG,   dstB,   dstA
	BLENDFACTOR_INV_DST_COLOR = 0x8,  // 1-dstR, 1-dstG, 1-dstB, 1-dstA
	BLENDFACTOR_DST_ALPHA     = 0x9,  //   dstA,   dstA,   dstA,   dstA
	BLENDFACTOR_INV_DST_ALPHA = 0xA,  // 1-dstA, 1-dstA, 1-dstA, 1-dstA
} BlendFactor;

typedef enum BlendMode {
	BLEND_NONE = BLENDMODE_COMPOSE(
		BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDOP_ADD,
		BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDOP_ADD
	),

	BLEND_ALPHA = BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_INV_SRC_ALPHA, BLENDOP_ADD,
		BLENDFACTOR_ONE,       BLENDFACTOR_INV_SRC_ALPHA, BLENDOP_ADD
	),

	BLEND_ADD = BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE, BLENDOP_ADD,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_ADD
	),

	BLEND_SUB = BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE, BLENDOP_REV_SUB,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_REV_SUB
	),

	BLEND_MOD = BLENDMODE_COMPOSE(
		BLENDFACTOR_ZERO, BLENDFACTOR_SRC_COLOR, BLENDOP_ADD,
		BLENDFACTOR_ZERO, BLENDFACTOR_ONE,       BLENDOP_ADD
	),
} BlendMode;

typedef struct UnpackedBlendModePart {
	BlendOp op;
	BlendFactor src;
	BlendFactor dst;
} UnpackedBlendModePart;

typedef struct UnpackedBlendMode {
	UnpackedBlendModePart color;
	UnpackedBlendModePart alpha;
} UnpackedBlendMode;

typedef enum CullFaceMode {
	CULL_FRONT  = 0x1,
	CULL_BACK   = 0x2,
	CULL_BOTH   = CULL_FRONT | CULL_BACK,
} CullFaceMode;

typedef enum DepthTestFunc {
	DEPTH_NEVER,
	DEPTH_ALWAYS,
	DEPTH_EQUAL,
	DEPTH_NOTEQUAL,
	DEPTH_LESS,
	DEPTH_LEQUAL,
	DEPTH_GREATER,
	DEPTH_GEQUAL,
} DepthTestFunc;

typedef enum VsyncMode {
	VSYNC_NONE,
	VSYNC_NORMAL,
	VSYNC_ADAPTIVE,
} VsyncMode;

typedef struct SpriteParams {
	const char *sprite;
	Sprite *sprite_ptr;

	const char *shader;
	ShaderProgram *shader_ptr;

	Color color;
	BlendMode blend;

	struct {
		float x;
		float y;
	} pos;

	struct {
		union {
			float x;
			float both;
		};

		float y;
	} scale;

	struct {
		float angle;
		vec3 vector;
	} rotation;

	float custom;

	struct {
		unsigned int x : 1;
		unsigned int y : 1;
	} flip;
} attr_designated_init SpriteParams;

/*
 * Creates an SDL window with proper flags, and, if needed, sets up a rendering context associated with it.
 * Must be called before anything else.
 */

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags)
	attr_nonnull(1) attr_nodiscard;

/*
 *	TODO: Document these, and put them in an order that makes a little bit of sense.
 */

void r_init(void);
void r_post_init(void);
void r_shutdown(void);

bool r_supports(RendererFeature feature);

void r_capability(RendererCapability cap, bool value);
bool r_capability_current(RendererCapability cap);

void r_color4(float r, float g, float b, float a);
Color r_color_current(void);

void r_blend(BlendMode mode);
BlendMode r_blend_current(void);

void r_cull(CullFaceMode mode);
CullFaceMode r_cull_current(void);

void r_depth_func(DepthTestFunc func);
DepthTestFunc r_depth_func_current(void);

void r_shader_ptr(ShaderProgram *prog) attr_nonnull(1);
ShaderProgram* r_shader_current(void) attr_returns_nonnull;

Uniform* r_shader_uniform(ShaderProgram *prog, const char *uniform_name) attr_nonnull(1, 2);
void r_uniform_ptr(Uniform *uniform, uint count, const void *data) attr_nonnull(3);
UniformType r_uniform_type(Uniform *uniform);

void r_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances, uint base_instance);

void r_texture_create(Texture *tex, const TextureParams *params) attr_nonnull(1, 2);
void r_texture_get_params(Texture *tex, TextureParams *params) attr_nonnull(1, 2);
void r_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) attr_nonnull(1);
void r_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) attr_nonnull(1);
void r_texture_fill(Texture *tex, uint mipmap, void *image_data) attr_nonnull(1, 3);
void r_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data) attr_nonnull(1, 7);
void r_texture_invalidate(Texture *tex) attr_nonnull(1);
void r_texture_destroy(Texture *tex) attr_nonnull(1);

void r_texture_ptr(uint unit, Texture *tex);
Texture* r_texture_current(uint unit);

void r_framebuffer_create(Framebuffer *fb) attr_nonnull(1);
void r_framebuffer_attach(Framebuffer *fb, Texture *tex, uint mipmap, FramebufferAttachment attachment) attr_nonnull(1);
Texture* r_framebuffer_get_attachment(Framebuffer *fb, FramebufferAttachment attachment) attr_nonnull(1);
uint r_framebuffer_get_attachment_mipmap(Framebuffer *fb, FramebufferAttachment attachment) attr_nonnull(1);
void r_framebuffer_viewport(Framebuffer *fb, int x, int y, int w, int h);
void r_framebuffer_viewport_rect(Framebuffer *fb, IntRect viewport);
void r_framebuffer_viewport_current(Framebuffer *fb, IntRect *viewport) attr_nonnull(2);
void r_framebuffer_destroy(Framebuffer *fb) attr_nonnull(1);

void r_framebuffer(Framebuffer *fb);
Framebuffer* r_framebuffer_current(void);

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data) attr_nonnull(1);
void r_vertex_buffer_destroy(VertexBuffer *vbuf) attr_nonnull(1);
void r_vertex_buffer_invalidate(VertexBuffer *vbuf) attr_nonnull(1);
void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) attr_nonnull(1, 4);
void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) attr_nonnull(1, 3);

void r_vertex_array_create(VertexArray *varr) attr_nonnull(1);
void r_vertex_array_destroy(VertexArray *varr) attr_nonnull(1);
void r_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) attr_nonnull(1, 2);
VertexBuffer* r_vertex_array_get_attachment(VertexArray *varr, uint attachment)  attr_nonnull(1);
void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) attr_nonnull(1, 3);

void r_vertex_array(VertexArray *varr) attr_nonnull(1);
VertexArray* r_vertex_array_current(void);

void r_clear(ClearBufferFlags flags);
void r_clear_color4(float r, float g, float b, float a);
Color r_clear_color_current(void);

void r_vsync(VsyncMode mode);
VsyncMode r_vsync_current(void);

void r_swap(SDL_Window *window);

uint8_t* r_screenshot(uint *out_width, uint *out_height) attr_nodiscard attr_nonnull(1, 2);

void r_mat_mode(MatrixMode mode);
MatrixMode r_mat_mode_current(void);
void r_mat_push(void);
void r_mat_pop(void);
void r_mat_identity(void);
void r_mat_translate_v(vec3 v);
void r_mat_rotate_v(float angle, vec3 v);
void r_mat_scale_v(vec3 v);
void r_mat_ortho(float left, float right, float bottom, float top, float near, float far);
void r_mat_perspective(float angle, float aspect, float near, float far);

void r_mat(MatrixMode mode, mat4 mat);
void r_mat_current(MatrixMode mode, mat4 out_mat);
mat4* r_mat_current_ptr(MatrixMode mode);

void r_shader_standard(void);
void r_shader_standard_notex(void);

VertexBuffer* r_vertex_buffer_static_models(void) attr_returns_nonnull;
VertexArray* r_vertex_array_static_models(void) attr_returns_nonnull;

void r_state_push(void);
void r_state_pop(void);

void r_draw_quad(void);
void r_draw_quad_instanced(uint instances);
void r_draw_model_ptr(Model *model) attr_nonnull(1);
void r_draw_sprite(const SpriteParams *params) attr_nonnull(1);

void r_flush_sprites(void);

BlendMode r_blend_compose(
	BlendFactor src_color, BlendFactor dst_color, BlendOp color_op,
	BlendFactor src_alpha, BlendFactor dst_alpha, BlendOp alpha_op
);

uint32_t r_blend_component(BlendMode mode, BlendModeComponent component);
void r_blend_unpack(BlendMode mode, UnpackedBlendMode *dest) attr_nonnull(2);

const UniformTypeInfo* r_uniform_type_info(UniformType type) attr_returns_nonnull;
const VertexAttribTypeInfo* r_vertex_attrib_type_info(VertexAttribType type);

VertexAttribFormat* r_vertex_attrib_format_interleaved(
	size_t nattribs,
	VertexAttribSpec specs[nattribs],
	VertexAttribFormat formats[nattribs],
	uint attachment
) attr_nonnull(2, 3);

/*
 * Small convenience wrappers
 */

static inline attr_must_inline
void r_enable(RendererCapability cap) {
	r_capability(cap, true);
}

static inline attr_must_inline
void r_disable(RendererCapability cap) {
	r_capability(cap, false);
}

static inline attr_must_inline
ShaderProgram* r_shader_get(const char *name) {
	return get_resource_data(RES_SHADER_PROGRAM, name, RESF_DEFAULT | RESF_UNSAFE);
}

static inline attr_must_inline
ShaderProgram* r_shader_get_optional(const char *name) {
	ShaderProgram *prog = get_resource_data(RES_SHADER_PROGRAM, name, RESF_OPTIONAL | RESF_UNSAFE);

	if(!prog) {
		log_warn("shader program %s could not be loaded", name);
	}

	return prog;
}

static inline attr_must_inline
Texture* r_texture_get(const char *name) {
	return get_resource_data(RES_TEXTURE, name, RESF_DEFAULT | RESF_UNSAFE);
}

static inline attr_must_inline
void r_mat_translate(float x, float y, float z) {
	r_mat_translate_v((vec3) { x, y, z });
}

static inline attr_must_inline
void r_mat_rotate(float angle, float nx, float ny, float nz) {
	r_mat_rotate_v(angle, (vec3) { nx, ny, nz });
}

static inline attr_must_inline
void r_mat_rotate_deg(float angle_degrees, float nx, float ny, float nz) {
	r_mat_rotate_v(angle_degrees * 0.017453292519943295, (vec3) { nx, ny, nz });
}

static inline attr_must_inline
void r_mat_rotate_deg_v(float angle_degrees, vec3 v) {
	r_mat_rotate_v(angle_degrees * 0.017453292519943295, v);
}

static inline attr_must_inline
void r_mat_scale(float sx, float sy, float sz) {
	r_mat_scale_v((vec3) { sx, sy, sz });
}

static inline attr_must_inline
void r_color(Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_color4(r, g, b, a);
}

static inline attr_must_inline
void r_color3(float r, float g, float b) {
	r_color4(r, g, b, 1.0);
}

static inline attr_must_inline attr_nonnull(1)
void r_shader(const char *prog) {
	r_shader_ptr(r_shader_get(prog));
}

static inline attr_must_inline
void r_texture(uint unit, const char *tex) {
	r_texture_ptr(unit, r_texture_get(tex));
}

static inline attr_must_inline
Uniform* r_shader_current_uniform(const char *name) {
	return r_shader_uniform(r_shader_current(), name);
}

static inline attr_must_inline
void r_uniform(const char *name, uint count, void *data) {
	r_uniform_ptr(r_shader_current_uniform(name), count, data);
}

static inline attr_must_inline
void r_uniform_float(const char *name, float v) {
	r_uniform(name, 1, (float[1]) { v });
}

static inline attr_must_inline
void r_uniform_int(const char *name, int v) {
	r_uniform(name, 1, (int[1]) { v });
}

static inline attr_must_inline
void r_uniform_vec2(const char *name, float x, float y) {
	r_uniform(name, 1, (float[2]) { x, y } );
}

static inline attr_must_inline
void r_uniform_vec3(const char *name, float x, float y, float z) {
	r_uniform(name, 1, (vec3) { x, y, z });
}

static inline attr_must_inline
void r_uniform_vec4(const char *name, float x, float y, float z, float w) {
	r_uniform(name, 1, (vec4) { x, y, z, w });
}

static inline attr_must_inline
void r_uniform_rgb(const char *name, Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_uniform_vec3(name, r, g, b);
}

static inline attr_must_inline
void r_uniform_rgba(const char *name, Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_uniform_vec4(name, r, g, b, a);
}

static inline attr_must_inline
void r_uniform_complex(const char *name, complex z) {
	r_uniform(name, 1, (float[]) { creal(z), cimag(z) });
}

static inline attr_must_inline
void r_uniform_complex_array(const char *name, uint count, complex z[count]) {
	float data[count * 2], *ptr = data;

	for(int i = 0; i < count; ++i) {
		*ptr++ = creal(z[i]);
		*ptr++ = cimag(z[i]);
	}

	r_uniform(name, count, data);
}

static inline attr_must_inline
void r_clear_color3(float r, float g, float b) {
	r_clear_color4(r, g, b, 1.0);
}

static inline attr_must_inline
void r_clear_color(Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_clear_color4(r, g, b, a);
}

static inline attr_must_inline attr_nonnull(1)
void r_draw_model(const char *model) {
	r_draw_model_ptr(get_resource_data(RES_MODEL, model, RESF_UNSAFE));
}

static inline attr_must_inline
r_capability_bits_t r_capability_bit(RendererCapability cap) {
	r_capability_bits_t idx = cap;
	assert(idx < NUM_RCAPS);
	return (1 << idx);
}

static inline attr_must_inline
r_feature_bits_t r_feature_bit(RendererFeature feat) {
	r_feature_bits_t idx = feat;
	assert(idx < NUM_RFEATS);
	return (1 << idx);
}
