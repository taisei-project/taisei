/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "pixmap/pixmap.h"
#include "color.h"
#include "common/shaderlib/shaderlib.h"
#include "resource/resource.h"
#include "resource/shader_program.h"
#include "resource/texture.h"

typedef struct Texture Texture;
typedef struct Framebuffer Framebuffer;
typedef struct VertexBuffer VertexBuffer;
typedef struct VertexArray VertexArray;
typedef struct IndexBuffer IndexBuffer;
typedef struct ShaderObject ShaderObject;
typedef struct ShaderProgram ShaderProgram;
typedef struct Sprite Sprite;
typedef struct Model Model;

enum {
	R_DEBUG_LABEL_SIZE = 128,
	R_NUM_SPRITE_AUX_TEXTURES = 3,
};

typedef enum RendererFeature {
	RFEAT_DRAW_INSTANCED,
	RFEAT_DRAW_INSTANCED_BASE_INSTANCE,
	RFEAT_DEPTH_TEXTURE,
	RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS,
	RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN,
	RFEAT_TEXTURE_SWIZZLE,
	RFEAT_PARTIAL_MIPMAPS,

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

enum {
	TEX_TYPE_COMPRESSED_BIT = 0x8000,
};

#define TEX_TYPES_UNCOMPRESSED(X, ...) \
	/* NOTE: whichever is placed first here is considered the "default" where applicable. */ \
	X(RGBA_8, __VA_ARGS__) \
	X(RGB_8, __VA_ARGS__) \
	X(RG_8, __VA_ARGS__) \
	X(R_8, __VA_ARGS__) \
	X(RGBA_16, __VA_ARGS__) \
	X(RGB_16, __VA_ARGS__) \
	X(RG_16, __VA_ARGS__) \
	X(R_16, __VA_ARGS__) \
	X(RGBA_16_FLOAT, __VA_ARGS__) \
	X(RGB_16_FLOAT, __VA_ARGS__) \
	X(RG_16_FLOAT, __VA_ARGS__) \
	X(R_16_FLOAT, __VA_ARGS__) \
	X(RGBA_32_FLOAT, __VA_ARGS__) \
	X(RGB_32_FLOAT, __VA_ARGS__) \
	X(RG_32_FLOAT, __VA_ARGS__) \
	X(R_32_FLOAT, __VA_ARGS__) \
	X(DEPTH_8, __VA_ARGS__) \
	X(DEPTH_16, __VA_ARGS__) \
	X(DEPTH_24, __VA_ARGS__) \
	X(DEPTH_32, __VA_ARGS__) \
	X(DEPTH_16_FLOAT, __VA_ARGS__) \
	X(DEPTH_32_FLOAT, __VA_ARGS__) \

#define TEX_TYPES_HANDLE_COMPRESSED(comp, layout, X, ...) \
	X(COMPRESSED_##comp, __VA_ARGS__)

#define TEX_TYPES_COMPRESSED(X, ...) \
	PIXMAP_COMPRESSION_FORMATS(TEX_TYPES_HANDLE_COMPRESSED, X, __VA_ARGS__)

#define TEX_TYPES(X, ...) \
	TEX_TYPES_UNCOMPRESSED(X, __VA_ARGS__) \
	TEX_TYPES_COMPRESSED(X, __VA_ARGS__) \

typedef enum TextureType {
	TEX_TYPE_INVALID = -1,

	#define DECLARE_TEX_UNCOMPRESSED_TYPE(type, ...) \
		TEX_TYPE_##type,
	TEX_TYPES_UNCOMPRESSED(DECLARE_TEX_UNCOMPRESSED_TYPE,)
	#undef DECLARE_TEX_UNCOMPRESSED_TYPE

	#define DECLARE_TEX_COMPRESSED_TYPE(fmt, ...) \
		TEX_TYPE_COMPRESSED_##fmt = PIXMAP_COMPRESSION_##fmt | TEX_TYPE_COMPRESSED_BIT,
	PIXMAP_COMPRESSION_FORMATS(DECLARE_TEX_COMPRESSED_TYPE,)
	#undef DECLARE_TEX_COMPRESSED_TYPE

	TEX_TYPE_R = TEX_TYPE_R_8,
	TEX_TYPE_RG = TEX_TYPE_RG_8,
	TEX_TYPE_RGB = TEX_TYPE_RGB_8,
	TEX_TYPE_RGBA = TEX_TYPE_RGBA_8,
	TEX_TYPE_DEPTH = TEX_TYPE_DEPTH_24,
} TextureType;

#define TEX_TYPE_IS_COMPRESSED(type)         ((bool)((type) & TEX_TYPE_COMPRESSED_BIT))
#define TEX_TYPE_TO_COMPRESSION_FORMAT(type) ((PixmapCompression)((type) & ~TEX_TYPE_COMPRESSED_BIT))
#define COMPRESSION_FORMAT_TO_TEX_TYPE(cfmt) ((TextureType)((cfmt) | TEX_TYPE_COMPRESSED_BIT))
#define TEX_TYPE_IS_DEPTH(type)              ((type) >= TEX_TYPE_DEPTH_8 && (type) <= TEX_TYPE_DEPTH_32_FLOAT)

typedef enum TextureClass {
	// NOTE: whichever is placed first here is considered the "default" where applicable.
	TEXTURE_CLASS_2D,
	TEXTURE_CLASS_CUBEMAP,
} TextureClass;

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

typedef enum TextureFlags {
	// NOTE: this can be later expanded to include intended usage flags, e.g. "render target", "filterable", etc.
	TEX_FLAG_SRGB = (1 << 0),
	TEX_FLAG_STREAM = (1 << 1),
} TextureFlags;

enum {
	TEX_ANISOTROPY_DEFAULT = 1,
	// TEX_MIPMAPS_MAX = ((uint)(-1)),
	// pedantic out-of-range warning
	#define TEX_MIPMAPS_MAX ((uint)(-1))
};

typedef enum CubemapFace {
	CUBEMAP_FACE_POS_X,
	CUBEMAP_FACE_NEG_X,
	CUBEMAP_FACE_POS_Y,
	CUBEMAP_FACE_NEG_Y,
	CUBEMAP_FACE_POS_Z,
	CUBEMAP_FACE_NEG_Z,
} CubemapFace;

typedef struct TextureParams {
	uint width;
	uint height;
	uint layers;
	TextureType type;
	TextureClass class;

	struct {
		TextureFilterMode mag;
		TextureFilterMode min;
	} filter;

	struct {
		TextureWrapMode s;
		TextureWrapMode t;
	} wrap;

	SwizzleMask swizzle;

	uint anisotropy;
	uint mipmaps;
	TextureMipmapMode mipmap_mode;
	TextureFlags flags;
} attr_designated_init TextureParams;

typedef struct TextureTypeQueryResult {
	PixmapFormat optimal_pixmap_format;
	PixmapOrigin optimal_pixmap_origin;
	bool supplied_pixmap_format_supported;
	bool supplied_pixmap_origin_supported;
} TextureTypeQueryResult;

typedef enum FramebufferAttachment {
	FRAMEBUFFER_ATTACH_DEPTH,
	FRAMEBUFFER_ATTACH_COLOR0,
	FRAMEBUFFER_ATTACH_COLOR1,
	FRAMEBUFFER_ATTACH_COLOR2,
	FRAMEBUFFER_ATTACH_COLOR3,

	FRAMEBUFFER_MAX_COLOR_ATTACHMENTS = 4,
	FRAMEBUFFER_MAX_ATTACHMENTS = FRAMEBUFFER_ATTACH_COLOR0 + FRAMEBUFFER_MAX_COLOR_ATTACHMENTS,
	FRAMEBUFFER_ATTACH_NONE = -1,
} FramebufferAttachment;

enum {
	FRAMEBUFFER_MAX_OUTPUTS = FRAMEBUFFER_MAX_COLOR_ATTACHMENTS,
};

typedef enum Primitive {
	PRIM_POINTS,
	PRIM_LINE_STRIP,
	PRIM_LINE_LOOP,
	PRIM_LINES,
	PRIM_TRIANGLE_STRIP,
	PRIM_TRIANGLES,
} Primitive;

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
	float position[3];
	float uv[2];
	float normal[3];
	float tangent[4];   // NOTE: bitangent = cross(normal, tangent) * tangent[3]
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
	UNIFORM_SAMPLER_2D,
	UNIFORM_SAMPLER_CUBE,
	UNIFORM_MAT3,
	UNIFORM_MAT4,
	UNIFORM_UNKNOWN,
} UniformType;

#define UNIFORM_TYPE_IS_SAMPLER(ut) \
	((ut) == UNIFORM_SAMPLER_2D || (ut) == UNIFORM_SAMPLER_CUBE)

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

	BLEND_PREMUL_ALPHA = BLENDMODE_COMPOSE(
		BLENDFACTOR_ONE, BLENDFACTOR_INV_SRC_ALPHA, BLENDOP_ADD,
		BLENDFACTOR_ONE, BLENDFACTOR_INV_SRC_ALPHA, BLENDOP_ADD
	),

	_BLEND_ADD = BLENDMODE_COMPOSE(
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

attr_deprecated("Use BLEND_PREMUL_ALPHA and a color with alpha == 0 instead")
static const BlendMode BLEND_ADD = _BLEND_ADD;

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

typedef union ShaderCustomParams {
	float vector[4];
	Color color;
} ShaderCustomParams;

typedef struct SpriteStateParams {
	Texture *primary_texture;
	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];
	BlendMode blend;
	ShaderProgram *shader;
} SpriteStateParams;

typedef union SpriteScaleParams {
	struct {
		union {
			float x;
			float both;
		};

		float y;
	};

	cmplxf as_cmplx;
} SpriteScaleParams;

typedef struct SpriteRotationParams {
	vec3 vector;
	float angle;
} SpriteRotationParams;

typedef struct SpriteFlipParams {
	unsigned char x : 1;
	unsigned char y : 1;
} SpriteFlipParams;

typedef struct SpriteParams {
	const char *sprite;
	Sprite *sprite_ptr;

	const char *shader;
	ShaderProgram *shader_ptr;

	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];

	// TODO: maybe embed these by value and get rid of SpriteParamsBuffer?
	const Color *color;
	const ShaderCustomParams *shader_params;

	BlendMode blend;

	FloatOffset pos;
	SpriteScaleParams scale;
	SpriteRotationParams rotation;
	SpriteFlipParams flip;
} SpriteParams;

typedef struct SpriteParamsBuffer {
	Color color;
	ShaderCustomParams shader_params;
} SpriteParamsBuffer;

// Matches vertex buffer layout
typedef struct SpriteInstanceAttribs {
	mat4 mv_transform;
	mat4 tex_transform;

	union {
		FloatRect texrect;
		vec4 texrect_vec4;
	};

	Color rgba;
	FloatExtent sprite_size;
	ShaderCustomParams custom;

	// offsetof(end_of_fields) == size without padding.
	char end_of_fields;
} SpriteInstanceAttribs;

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
const char *r_backend_name(void);

r_feature_bits_t r_features(void);

r_capability_bits_t r_capabilities_current(void);
void r_capabilities(r_capability_bits_t newcaps);

void r_capability(RendererCapability cap, bool value);
bool r_capability_current(RendererCapability cap);

void r_color4(float r, float g, float b, float a);
const Color* r_color_current(void);

void r_blend(BlendMode mode);
BlendMode r_blend_current(void);

void r_cull(CullFaceMode mode);
CullFaceMode r_cull_current(void);

void r_depth_func(DepthTestFunc func);
DepthTestFunc r_depth_func_current(void);

bool r_shader_language_supported(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative) attr_nonnull(1);

ShaderObject* r_shader_object_compile(ShaderSource *source) attr_nonnull(1);
void r_shader_object_destroy(ShaderObject *shobj) attr_nonnull(1);
void r_shader_object_set_debug_label(ShaderObject *shobj, const char *label) attr_nonnull(1);
const char* r_shader_object_get_debug_label(ShaderObject *shobj) attr_nonnull(1);

ShaderProgram* r_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) attr_nonnull(2);
void r_shader_program_destroy(ShaderProgram *prog);
void r_shader_program_set_debug_label(ShaderProgram *prog, const char *label) attr_nonnull(1);
const char* r_shader_program_get_debug_label(ShaderProgram *prog) attr_nonnull(1);

void r_shader_ptr(ShaderProgram *prog) attr_nonnull(1);
ShaderProgram* r_shader_current(void) attr_returns_nonnull;

Uniform* _r_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash) attr_nonnull(1, 2);
UniformType r_uniform_type(Uniform *uniform);
void r_uniform_ptr_unsafe(Uniform *uniform, uint offset, uint count, void *data);

#define _R_UNIFORM_GENERIC(suffix, uniform, ...) (_Generic((uniform), \
	char* : _r_uniform_##suffix, \
	const char* : _r_uniform_##suffix, \
	Uniform* : _r_uniform_ptr_##suffix \
))(uniform, __VA_ARGS__)

void _r_uniform_ptr_float(Uniform *uniform, float value);
void _r_uniform_float(const char *uniform, float value) attr_nonnull(1);
#define r_uniform_float(uniform, ...) _R_UNIFORM_GENERIC(float, uniform, __VA_ARGS__)

void _r_uniform_ptr_float_array(Uniform *uniform, uint offset, uint count, float elements[count]) attr_nonnull(4);
void _r_uniform_float_array(const char *uniform, uint offset, uint count, float elements[count]) attr_nonnull(1, 4);
#define r_uniform_float_array(uniform, ...) _R_UNIFORM_GENERIC(float_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec2(Uniform *uniform, float x, float y);
void _r_uniform_vec2(const char *uniform, float x, float y) attr_nonnull(1);
#define r_uniform_vec2(uniform, ...) _R_UNIFORM_GENERIC(vec2, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec2_vec(Uniform *uniform, vec2_noalign value) attr_nonnull(2);
void _r_uniform_vec2_vec(const char *uniform, vec2_noalign value) attr_nonnull(1, 2);
#define r_uniform_vec2_vec(uniform, ...) _R_UNIFORM_GENERIC(vec2_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec2_complex(Uniform *uniform, cmplx value);
void _r_uniform_vec2_complex(const char *uniform, cmplx value) attr_nonnull(1);
#define r_uniform_vec2_complex(uniform, ...) _R_UNIFORM_GENERIC(vec2_complex, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec2_array(Uniform *uniform, uint offset, uint count, vec2_noalign elements[count]) attr_nonnull(4);
void _r_uniform_vec2_array(const char *uniform, uint offset, uint count, vec2_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_vec2_array(uniform, ...) _R_UNIFORM_GENERIC(vec2_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec2_array_complex(Uniform *uniform, uint offset, uint count, cmplx elements[count]) attr_nonnull(4);
void _r_uniform_vec2_array_complex(const char *uniform, uint offset, uint count, cmplx elements[count]) attr_nonnull(1, 4);
#define r_uniform_vec2_array_complex(uniform, ...) _R_UNIFORM_GENERIC(vec2_array_complex, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec3(Uniform *uniform, float x, float y, float z);
void _r_uniform_vec3(const char *uniform, float x, float y, float z) attr_nonnull(1);
#define r_uniform_vec3(uniform, ...) _R_UNIFORM_GENERIC(vec3, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec3_vec(Uniform *uniform, vec3_noalign value) attr_nonnull(2);
void _r_uniform_vec3_vec(const char *uniform, vec3_noalign value) attr_nonnull(1, 2);
#define r_uniform_vec3_vec(uniform, ...) _R_UNIFORM_GENERIC(vec3_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec3_rgb(Uniform *uniform, const Color *rgb) attr_nonnull(2);
void _r_uniform_vec3_rgb(const char *uniform, const Color *rgb) attr_nonnull(1, 2);
#define r_uniform_vec3_rgb(uniform, ...) _R_UNIFORM_GENERIC(vec3_rgb, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec3_array(Uniform *uniform, uint offset, uint count, vec3_noalign elements[count]) attr_nonnull(4);
void _r_uniform_vec3_array(const char *uniform, uint offset, uint count, vec3_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_vec3_array(uniform, ...) _R_UNIFORM_GENERIC(vec3_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec4(Uniform *uniform, float x, float y, float z, float w);
void _r_uniform_vec4(const char *uniform, float x, float y, float z, float w) attr_nonnull(1);
#define r_uniform_vec4(uniform, ...) _R_UNIFORM_GENERIC(vec4, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec4_vec(Uniform *uniform, vec4_noalign value) attr_nonnull(2);
void _r_uniform_vec4_vec(const char *uniform, vec4_noalign value) attr_nonnull(1, 2);
#define r_uniform_vec4_vec(uniform, ...) _R_UNIFORM_GENERIC(vec4_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec4_rgba(Uniform *uniform, const Color *rgba) attr_nonnull(2);
void _r_uniform_vec4_rgba(const char *uniform, const Color *rgba) attr_nonnull(1, 2);
#define r_uniform_vec4_rgba(uniform, ...) _R_UNIFORM_GENERIC(vec4_rgba, uniform, __VA_ARGS__)

void _r_uniform_ptr_vec4_array(Uniform *uniform, uint offset, uint count, vec4_noalign elements[count]) attr_nonnull(4);
void _r_uniform_vec4_array(const char *uniform, uint offset, uint count, vec4_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_vec4_array(uniform, ...) _R_UNIFORM_GENERIC(vec4_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_mat3(Uniform *uniform, mat3_noalign value) attr_nonnull(2);
void _r_uniform_mat3(const char *uniform, mat3_noalign value) attr_nonnull(1, 2);
#define r_uniform_mat3(uniform, ...) _R_UNIFORM_GENERIC(mat3, uniform, __VA_ARGS__)

void _r_uniform_ptr_mat3_array(Uniform *uniform, uint offset, uint count, mat3_noalign elements[count]) attr_nonnull(4);
void _r_uniform_mat3_array(const char *uniform, uint offset, uint count, mat3_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_mat3_array(uniform, ...) _R_UNIFORM_GENERIC(mat3_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_mat4(Uniform *uniform, mat4_noalign value) attr_nonnull(2);
void _r_uniform_mat4(const char *uniform, mat4_noalign value) attr_nonnull(1, 2);
#define r_uniform_mat4(uniform, ...) _R_UNIFORM_GENERIC(mat4, uniform, __VA_ARGS__)

void _r_uniform_ptr_mat4_array(Uniform *uniform, uint offset, uint count, mat4_noalign elements[count]) attr_nonnull(4);
void _r_uniform_mat4_array(const char *uniform, uint offset, uint count, mat4_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_mat4_array(uniform, ...) _R_UNIFORM_GENERIC(mat4_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_int(Uniform *uniform, int value);
void _r_uniform_int(const char *uniform, int value) attr_nonnull(1);
#define r_uniform_int(uniform, ...) _R_UNIFORM_GENERIC(int, uniform, __VA_ARGS__)

void _r_uniform_ptr_int_array(Uniform *uniform, uint offset, uint count, int elements[count]) attr_nonnull(4);
void _r_uniform_int_array(const char *uniform, uint offset, uint count, int elements[count]) attr_nonnull(1, 4);
#define r_uniform_int_array(uniform, ...) _R_UNIFORM_GENERIC(int_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec2(Uniform *uniform, int x, int y);
void _r_uniform_ivec2(const char *uniform, int x, int y) attr_nonnull(1);
#define r_uniform_ivec2(uniform, ...) _R_UNIFORM_GENERIC(ivec2, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec2_vec(Uniform *uniform, ivec2_noalign value) attr_nonnull(2);
void _r_uniform_ivec2_vec(const char *uniform, ivec2_noalign value) attr_nonnull(1, 2);
#define r_uniform_ivec2_vec(uniform, ...) _R_UNIFORM_GENERIC(ivec2_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec2_array(Uniform *uniform, uint offset, uint count, ivec2_noalign elements[count]) attr_nonnull(4);
void _r_uniform_ivec2_array(const char *uniform, uint offset, uint count, ivec2_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_ivec2_array(uniform, ...) _R_UNIFORM_GENERIC(ivec2_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec3(Uniform *uniform, int x, int y, int z);
void _r_uniform_ivec3(const char *uniform, int x, int y, int z) attr_nonnull(1);
#define r_uniform_ivec3(uniform, ...) _R_UNIFORM_GENERIC(ivec3, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec3_vec(Uniform *uniform, ivec3_noalign value) attr_nonnull(2);
void _r_uniform_ivec3_vec(const char *uniform, ivec3_noalign value) attr_nonnull(1, 2);
#define r_uniform_ivec3_vec(uniform, ...) _R_UNIFORM_GENERIC(ivec3_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec3_array(Uniform *uniform, uint offset, uint count, ivec3_noalign elements[count]) attr_nonnull(4);
void _r_uniform_ivec3_array(const char *uniform, uint offset, uint count, ivec3_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_ivec3_array(uniform, ...) _R_UNIFORM_GENERIC(ivec3_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec4(Uniform *uniform, int x, int y, int z, int w);
void _r_uniform_ivec4(const char *uniform, int x, int y, int z, int w) attr_nonnull(1);
#define r_uniform_ivec4(uniform, ...) _R_UNIFORM_GENERIC(ivec4, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec4_vec(Uniform *uniform, ivec4_noalign value) attr_nonnull(2);
void _r_uniform_ivec4_vec(const char *uniform, ivec4_noalign value) attr_nonnull(1, 2);
#define r_uniform_ivec4_vec(uniform, ...) _R_UNIFORM_GENERIC(ivec4_vec, uniform, __VA_ARGS__)

void _r_uniform_ptr_ivec4_array(Uniform *uniform, uint offset, uint count, ivec4_noalign elements[count]) attr_nonnull(4);
void _r_uniform_ivec4_array(const char *uniform, uint offset, uint count, ivec4_noalign elements[count]) attr_nonnull(1, 4);
#define r_uniform_ivec4_array(uniform, ...) _R_UNIFORM_GENERIC(ivec4_array, uniform, __VA_ARGS__)

void _r_uniform_ptr_sampler_ptr(Uniform *uniform, Texture *tex) attr_nonnull(2);
void _r_uniform_sampler_ptr(const char *uniform, Texture *tex) attr_nonnull(1, 2);
void _r_uniform_ptr_sampler(Uniform *uniform, const char *tex) attr_nonnull(2);
void _r_uniform_sampler(const char *uniform, const char *tex) attr_nonnull(1, 2);
#define r_uniform_sampler(uniform, tex) (_Generic((uniform), \
	char*         : _Generic((tex), \
			char*       : _r_uniform_sampler, \
			const char* : _r_uniform_sampler, \
			Texture*    : _r_uniform_sampler_ptr \
	), \
	const char*   : _Generic((tex), \
			char*       : _r_uniform_sampler, \
			const char* : _r_uniform_sampler, \
			Texture*    : _r_uniform_sampler_ptr \
	), \
	Uniform*      : _Generic((tex), \
			char*       : _r_uniform_ptr_sampler, \
			const char* : _r_uniform_ptr_sampler, \
			Texture*    : _r_uniform_ptr_sampler_ptr \
	) \
))(uniform, tex)

void _r_uniform_ptr_sampler_array_ptr(Uniform *uniform, uint offset, uint count, Texture *values[count]) attr_nonnull(4);
void _r_uniform_sampler_array_ptr(const char *uniform, uint offset, uint count, Texture *values[count]) attr_nonnull(1, 4);
void _r_uniform_ptr_sampler_array(Uniform *uniform, uint offset, uint count, const char *values[count]) attr_nonnull(4);
void _r_uniform_sampler_array(const char *uniform, uint offset, uint count, const char *values[count]) attr_nonnull(4);
#define r_uniform_sampler_array(uniform, offset, count, values) (_Generic(uniform, \
	char*        : _Generic((values), \
			char**    : _r_uniform_sampler_array, \
			Texture** : _r_uniform_sampler_array_ptr \
	), \
	const char*  : _Generic((values), \
			char**    : _r_uniform_sampler_array, \
			Texture** : _r_uniform_sampler_array_ptr \
	), \
	Uniform*     : _Generic((values), \
			char**    : _r_uniform_ptr_sampler_array, \
			Texture** : _r_uniform_ptr_sampler_array_ptr \
	) \
))(uniform, offset, count, values)

void r_draw(VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance);
void r_draw_indexed(VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance);

Texture* r_texture_create(const TextureParams *params) attr_nonnull(1);
void r_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) attr_nonnull(1);
uint r_texture_get_width(Texture *tex, uint mipmap) attr_nonnull(1);
uint r_texture_get_height(Texture *tex, uint mipmap) attr_nonnull(1);
void r_texture_get_params(Texture *tex, TextureParams *params) attr_nonnull(1, 2);
const char* r_texture_get_debug_label(Texture *tex) attr_nonnull(1);
void r_texture_set_debug_label(Texture *tex, const char *label) attr_nonnull(1);
void r_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) attr_nonnull(1);
void r_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) attr_nonnull(1);
void r_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image_data) attr_nonnull(1, 4);
void r_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image_data) attr_nonnull(1, 6);
bool r_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst) attr_nonnull(1, 4);
void r_texture_invalidate(Texture *tex) attr_nonnull(1);
void r_texture_clear(Texture *tex, const Color *clr) attr_nonnull(1, 2);
void r_texture_destroy(Texture *tex) attr_nonnull(1);

bool r_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result) attr_nodiscard;
const char *r_texture_type_name(TextureType type);
TextureType r_texture_type_from_pixmap_format(PixmapFormat fmt);

uint r_texture_util_max_num_miplevels(uint width, uint height);

Framebuffer* r_framebuffer_create(void);
const char* r_framebuffer_get_debug_label(Framebuffer *fb) attr_nonnull(1);
void r_framebuffer_set_debug_label(Framebuffer *fb, const char* label) attr_nonnull(1);
void r_framebuffer_attach(Framebuffer *fb, Texture *tex, uint mipmap, FramebufferAttachment attachment) attr_nonnull(1);
Texture* r_framebuffer_get_attachment(Framebuffer *fb, FramebufferAttachment attachment) attr_nonnull(1);
uint r_framebuffer_get_attachment_mipmap(Framebuffer *fb, FramebufferAttachment attachment) attr_nonnull(1);
void r_framebuffer_set_output_attachment(Framebuffer *fb, uint output, FramebufferAttachment attachment);
FramebufferAttachment r_framebuffer_get_output_attachment(Framebuffer *fb, uint output);
void r_framebuffer_set_output_attachments(Framebuffer *fb, const FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS]);
void r_framebuffer_get_output_attachments(Framebuffer *fb, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS]);
void r_framebuffer_viewport(Framebuffer *fb, float x, float y, float w, float h);
void r_framebuffer_viewport_rect(Framebuffer *fb, FloatRect viewport);
void r_framebuffer_viewport_current(Framebuffer *fb, FloatRect *viewport) attr_nonnull(2);
void r_framebuffer_destroy(Framebuffer *fb) attr_nonnull(1);
void r_framebuffer_clear(Framebuffer *fb, ClearBufferFlags flags, const Color *colorval, float depthval);
IntExtent r_framebuffer_get_size(Framebuffer *fb);

void r_framebuffer(Framebuffer *fb);
Framebuffer* r_framebuffer_current(void);

VertexBuffer* r_vertex_buffer_create(size_t capacity, void *data);
const char* r_vertex_buffer_get_debug_label(VertexBuffer *vbuf) attr_nonnull(1);
void r_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char* label) attr_nonnull(1);
void r_vertex_buffer_destroy(VertexBuffer *vbuf) attr_nonnull(1);
void r_vertex_buffer_invalidate(VertexBuffer *vbuf) attr_nonnull(1);
SDL_RWops* r_vertex_buffer_get_stream(VertexBuffer *vbuf) attr_nonnull(1);

IndexBuffer* r_index_buffer_create(size_t max_elements);
size_t r_index_buffer_get_capacity(IndexBuffer *ibuf) attr_nonnull(1);
const char* r_index_buffer_get_debug_label(IndexBuffer *ibuf) attr_nonnull(1);
void r_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) attr_nonnull(1);
void r_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) attr_nonnull(1);
size_t r_index_buffer_get_offset(IndexBuffer *ibuf) attr_nonnull(1);
void r_index_buffer_add_indices(IndexBuffer *ibuf, uint index_ofs, size_t num_indices, uint indices[num_indices]) attr_nonnull(1, 4);
void r_index_buffer_destroy(IndexBuffer *ibuf) attr_nonnull(1);

VertexArray* r_vertex_array_create(void);
const char* r_vertex_array_get_debug_label(VertexArray *varr) attr_nonnull(1);
void r_vertex_array_set_debug_label(VertexArray *varr, const char* label) attr_nonnull(1);
void r_vertex_array_destroy(VertexArray *varr) attr_nonnull(1);
void r_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) attr_nonnull(1, 2);
VertexBuffer* r_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment)  attr_nonnull(1);
void r_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf) attr_nonnull(1);
IndexBuffer* r_vertex_array_get_index_attachment(VertexArray *varr)  attr_nonnull(1);
void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) attr_nonnull(1, 3);

void r_model_add_static(Model *out_mdl, Primitive prim, size_t num_vertices, GenericModelVertex vertices[num_vertices], size_t num_indices, uint indices[num_indices]);
const Model *r_model_get_quad(void) attr_returns_nonnull;

void r_vsync(VsyncMode mode);
VsyncMode r_vsync_current(void);

void r_swap(SDL_Window *window);

bool r_screenshot(Pixmap *dest) attr_nodiscard attr_nonnull(1);

void r_mat_mv_push(void);
void r_mat_mv_push_premade(mat4 mat);
void r_mat_mv_push_identity(void);
void r_mat_mv_pop(void);
void r_mat_mv(mat4 mat);
void r_mat_mv_current(mat4 out_mat);
mat4 *r_mat_mv_current_ptr(void);
void r_mat_mv_identity(void);
void r_mat_mv_translate_v(vec3 v);
void r_mat_mv_rotate_v(float angle, vec3 v);
void r_mat_mv_scale_v(vec3 v);

void r_mat_proj_push(void);
void r_mat_proj_push_premade(mat4 mat);
void r_mat_proj_push_identity(void);
void r_mat_proj_push_ortho_ex(float left, float right, float bottom, float top, float near, float far);
void r_mat_proj_push_ortho(float width, float height);
void r_mat_proj_push_perspective(float angle, float aspect, float near, float far);
void r_mat_proj_pop(void);
void r_mat_proj(mat4 mat);
void r_mat_proj_current(mat4 out_mat);
mat4 *r_mat_proj_current_ptr(void);
void r_mat_proj_identity(void);
void r_mat_proj_translate_v(vec3 v);
void r_mat_proj_rotate_v(float angle, vec3 v);
void r_mat_proj_scale_v(vec3 v);
void r_mat_proj_ortho(float left, float right, float bottom, float top, float near, float far);
void r_mat_proj_perspective(float angle, float aspect, float near, float far);

void r_mat_tex_push(void);
void r_mat_tex_push_premade(mat4 mat);
void r_mat_tex_push_identity(void);
void r_mat_tex_pop(void);
void r_mat_tex(mat4 mat);
void r_mat_tex_current(mat4 out_mat);
mat4 *r_mat_tex_current_ptr(void);
void r_mat_tex_identity(void);
void r_mat_tex_translate_v(vec3 v);
void r_mat_tex_rotate_v(float angle, vec3 v);
void r_mat_tex_scale_v(vec3 v);

void r_shader_standard(void);
void r_shader_standard_notex(void);

VertexBuffer* r_vertex_buffer_static_models(void) attr_returns_nonnull;
VertexArray* r_vertex_array_static_models(void) attr_returns_nonnull;

void r_state_push(void);
void r_state_pop(void);

void r_draw_quad(void);
void r_draw_quad_instanced(uint instances);
void r_draw_model_ptr(const Model *model, uint instances, uint base_instance) attr_nonnull(1);
void r_draw_sprite(const SpriteParams *params) attr_nonnull(1);

void r_sprite_batch_prepare_state(const SpriteStateParams *stp);
void r_sprite_batch_add_instance(const SpriteInstanceAttribs *attribs);

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

INLINE
void r_enable(RendererCapability cap) {
	r_capability(cap, true);
}

INLINE
void r_disable(RendererCapability cap) {
	r_capability(cap, false);
}

DEFINE_DEPRECATED_RESOURCE_GETTER(ShaderProgram, r_shader_get, res_shader)
DEFINE_DEPRECATED_RESOURCE_GETTER(ShaderProgram, r_shader_get_optional, res_shader_optional)
DEFINE_DEPRECATED_RESOURCE_GETTER(Texture, r_texture_get, res_texture)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

/*
 * FIXME: These sometimes trip false positives in ASan (stack use after scope) with clang;
 * possibly only in unoptimized builds.
 */
#if 0

INLINE void r_mat_mv_translate(float x, float y, float z) { r_mat_mv_translate_v((vec3) { x, y, z }); }
INLINE void r_mat_proj_translate(float x, float y, float z) { r_mat_proj_translate_v((vec3) { x, y, z }); }
INLINE void r_mat_tex_translate(float x, float y, float z) { r_mat_tex_translate_v((vec3) { x, y, z }); }

INLINE void r_mat_mv_rotate(float angle, float nx, float ny, float nz) { r_mat_mv_rotate_v(angle, (vec3) { nx, ny, nz }); }
INLINE void r_mat_proj_rotate(float angle, float nx, float ny, float nz) { r_mat_proj_rotate_v(angle, (vec3) { nx, ny, nz }); }
INLINE void r_mat_tex_rotate(float angle, float nx, float ny, float nz) { r_mat_tex_rotate_v(angle, (vec3) { nx, ny, nz }); }

INLINE void r_mat_mv_scale(float sx, float sy, float sz) { r_mat_mv_scale_v((vec3) { sx, sy, sz }); }
INLINE void r_mat_proj_scale(float sx, float sy, float sz) { r_mat_proj_scale_v((vec3) { sx, sy, sz }); }
INLINE void r_mat_tex_scale(float sx, float sy, float sz) { r_mat_tex_scale_v((vec3) { sx, sy, sz }); }

#else

#define r_mat_mv_translate(x, y, z)   r_mat_mv_translate_v((vec3) { (x), (y), (z) })
#define r_mat_proj_translate(x, y, z) r_mat_proj_translate_v((vec3) { (x), (y), (z) })
#define r_mat_tex_translate(x, y, z)  r_mat_tex_translate_v((vec3) { (x), (y), (z) })

#define r_mat_mv_scale(x, y, z)       r_mat_mv_scale_v((vec3) { (x), (y), (z) })
#define r_mat_proj_scale(x, y, z)     r_mat_proj_scale_v((vec3) { (x), (y), (z) })
#define r_mat_tex_scale(x, y, z)      r_mat_tex_scale_v((vec3) { (x), (y), (z) })

#define r_mat_mv_rotate(a, x, y, z)   r_mat_mv_rotate_v((a), (vec3) { (x), (y), (z) })
#define r_mat_proj_rotate(a, x, y, z) r_mat_proj_rotate_v((a), (vec3) { (x), (y), (z) })
#define r_mat_tex_rotate(a, x, y, z)  r_mat_tex_rotate_v((a), (vec3) { (x), (y), (z) })

#endif

#pragma GCC diagnostic pop

INLINE
void r_color(const Color *c) {
	r_color4(c->r, c->g, c->b, c->a);
}

INLINE
void r_color3(float r, float g, float b) {
	r_color4(r, g, b, 1.0);
}

INLINE attr_nonnull(1)
void r_shader(const char *prog) {
	r_shader_ptr(res_shader(prog));
}

INLINE
Uniform* r_shader_uniform(ShaderProgram *prog, const char *name) {
	return _r_shader_uniform(prog, name, ht_str2ptr_hash(name));
}

INLINE
Uniform* r_shader_current_uniform(const char *name) {
	return r_shader_uniform(r_shader_current(), name);
}

INLINE
void r_clear(ClearBufferFlags flags, const Color *colorval, float depthval) {
	r_framebuffer_clear(r_framebuffer_current(), flags, colorval, depthval);
}

INLINE attr_nonnull(1)
void r_draw_model(const char *model) {
	r_draw_model_ptr(get_resource_data(RES_MODEL, model, RESF_DEFAULT), 0, 0);
}

INLINE attr_nonnull(1)
void r_draw_model_instanced(const char *model, uint instances, uint base_instance) {
	r_draw_model_ptr(get_resource_data(RES_MODEL, model, RESF_DEFAULT), instances, base_instance);
}

INLINE
r_capability_bits_t r_capability_bit(RendererCapability cap) {
	r_capability_bits_t idx = cap;
	assert(idx < NUM_RCAPS);
	return (1 << idx);
}

INLINE
r_feature_bits_t r_feature_bit(RendererFeature feat) {
	r_feature_bits_t idx = feat;
	assert(idx < NUM_RFEATS);
	return (1 << idx);
}

INLINE
bool r_supports(RendererFeature feature) {
	return r_features() & r_feature_bit(feature);
}
