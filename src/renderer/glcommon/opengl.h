/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>

#ifdef TAISEI_BUILDCONF_RENDERER_STATIC_GLES30
	#define STATIC_GLES3
#endif

#ifdef STATIC_GLES3
	#define GL_GLEXT_PROTOTYPES
	#include <GLES3/gl32.h>
	#include <GLES2/gl2ext.h>

	// XXX: Workaround for https://github.com/emscripten-core/emscripten/issues/11801
	#ifndef GL_EXT_texture_compression_rgtc
	#define GL_EXT_texture_compression_rgtc 1
	#define GL_COMPRESSED_RED_RGTC1_EXT       0x8DBB
	#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
	#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
	#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
	#endif /* GL_EXT_texture_compression_rgtc */

	#ifndef GL_EXT_texture_compression_bptc
	#define GL_EXT_texture_compression_bptc 1
	#define GL_COMPRESSED_RGBA_BPTC_UNORM_EXT 0x8E8C
	#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT 0x8E8D
	#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_EXT 0x8E8E
	#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_EXT 0x8E8F
	#endif /* GL_EXT_texture_compression_bptc */

	#ifndef GL_EXT_texture_compression_s3tc_srgb
	#define GL_EXT_texture_compression_s3tc_srgb 1
	#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
	#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
	#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
	#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
	#endif /* GL_EXT_texture_compression_s3tc_srgb */

	#ifndef GL_3DFX_texture_compression_FXT1
	#define GL_3DFX_texture_compression_FXT1 1
	#define GL_COMPRESSED_RGB_FXT1_3DFX       0x86B0
	#define GL_COMPRESSED_RGBA_FXT1_3DFX      0x86B1
	#endif /* GL_3DFX_texture_compression_FXT1 */

	typedef PFNGLOBJECTLABELKHRPROC PFNGLOBJECTLABELPROC;
	typedef PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXTPROC PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC;
	typedef PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXTPROC PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC;
	typedef PFNGLVIEWPORTINDEXEDFVOESPROC PFNGLVIEWPORTINDEXEDFVPROC;
	typedef PFNGLGETFLOATI_VNVPROC PFNGLGETFLOATI_VPROC;
	typedef PFNGLCLEARTEXIMAGEEXTPROC PFNGLCLEARTEXIMAGEPROC;

	#define GL_FUNC(func) (gl##func)

	#ifndef APIENTRY
	#define APIENTRY GL_APIENTRY
	#endif

	#define GL_DEPTH_COMPONENT32 GL_DEPTH_COMPONENT32_OES
	#define GL_R16 GL_R16_EXT
	#define GL_R16_SNORM GL_R16_SNORM_EXT
	#define GL_RG16 GL_RG16_EXT
	#define GL_RG16_SNORM GL_RG16_SNORM_EXT
	#define GL_RGB10 GL_RGB10_EXT
	#define GL_RGB16 GL_RGB16_EXT
	#define GL_RGB16_SNORM GL_RGB16_SNORM_EXT
	#define GL_RGBA16 GL_RGBA16_EXT
	#define GL_RGBA16_SNORM GL_RGBA16_SNORM_EXT
	#define GL_TEXTURE_MAX_ANISOTROPY GL_TEXTURE_MAX_ANISOTROPY_EXT
	#define GL_COMPRESSED_RED_RGTC1 GL_COMPRESSED_RED_RGTC1_EXT
	#define GL_COMPRESSED_RG_RGTC2 GL_COMPRESSED_RED_GREEN_RGTC2_EXT
	#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB GL_COMPRESSED_RGBA_BPTC_UNORM_EXT
	#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT

	#define glClearDepth glClearDepthf
	#define glClearTexImage glClearTexImageEXT
#else
	#include <glad/gl.h>
	#define GL_FUNC(f) (glad_gl##f)

	#ifndef APIENTRY
		#define APIENTRY GLAD_API_PTR
	#endif
#endif

#ifndef GL_ANGLE_request_extension
#define GL_ANGLE_request_extension 1
#define GL_REQUESTABLE_EXTENSIONS_ANGLE 0x93A8
#define GL_NUM_REQUESTABLE_EXTENSIONS_ANGLE 0x93A9
typedef void (APIENTRY *PFNGLREQUESTEXTENSIONANGLEPROC) (const GLchar *name);
typedef void (APIENTRY *PFNGLDISABLEEXTENSIONANGLEPROC) (const GLchar *name);
#endif /* GL_ANGLE_request_extension */

// NOTE: The ability to query supported GLSL versions was added in GL 4.3,
// but it's not exposed by any extension. This is pretty silly.
#ifndef GL_NUM_SHADING_LANGUAGE_VERSIONS
	#define GL_NUM_SHADING_LANGUAGE_VERSIONS  0x82E9
#endif

#define TSGL_EXT_VENDORS \
	TSGL_EXT_VENDOR(3DFX) \
	TSGL_EXT_VENDOR(AMD) \
	TSGL_EXT_VENDOR(ANGLE) \
	TSGL_EXT_VENDOR(APPLE) \
	TSGL_EXT_VENDOR(ARB) \
	TSGL_EXT_VENDOR(EXT) \
	TSGL_EXT_VENDOR(IMG) \
	TSGL_EXT_VENDOR(KHR) \
	TSGL_EXT_VENDOR(MESA) \
	TSGL_EXT_VENDOR(NATIVE) \
	TSGL_EXT_VENDOR(NV) \
	TSGL_EXT_VENDOR(OES) \
	TSGL_EXT_VENDOR(SGIX) \
	TSGL_EXT_VENDOR(WEBGL) \

enum {
	#define TSGL_EXT_VENDOR(v) _TSGL_EXTVNUM_##v,
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR
};

typedef enum {
	#define TSGL_EXT_VENDOR(v) TSGL_EXTFLAG_##v = (1 << _TSGL_EXTVNUM_##v),
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR
} ext_flag_t;

static_assert(TSGL_EXTFLAG_KHR == (1 << _TSGL_EXTVNUM_KHR), "");

struct glext_s; // defined at the very bottom
extern struct glext_s glext;

void glcommon_load_library(void);
void glcommon_load_functions(void);
void glcommon_check_capabilities(void);
void glcommon_unload_library(void);
ext_flag_t glcommon_check_extension(const char *ext) attr_nonnull_all;
void glcommon_setup_attributes(SDL_GLprofile profile, uint major, uint minor, SDL_GLcontextFlag ctxflags);

struct glext_s {
	struct {
		char major;
		char minor;
		bool is_es;
		bool is_ANGLE;
		bool is_webgl;
	} version;

	struct {
		bool disable_norm16 : 1;
	} issues;

	ext_flag_t clear_texture;
	ext_flag_t color_buffer_float;
	ext_flag_t debug_output;
	ext_flag_t depth_texture;
	ext_flag_t draw_buffers;
	ext_flag_t float_blend;
	ext_flag_t instanced_arrays;
	ext_flag_t internalformat_query2;
	ext_flag_t invalidate_subdata;
	ext_flag_t pixel_buffer_object;
	ext_flag_t seamless_cubemap;
	ext_flag_t texture_filter_anisotropic;
	ext_flag_t texture_float;
	ext_flag_t texture_float_linear;
	ext_flag_t texture_half_float;
	ext_flag_t texture_half_float_linear;
	ext_flag_t texture_norm16;
	ext_flag_t texture_rg;
	ext_flag_t texture_storage;
	ext_flag_t texture_swizzle;
	ext_flag_t vertex_array_object;
	ext_flag_t viewport_array;

	struct {
		ext_flag_t r8_srgb;
		ext_flag_t rg8_srgb;
		ext_flag_t rgb8_rgba8_srgb;
		ext_flag_t astc;
		ext_flag_t atc;
		ext_flag_t bptc;
		ext_flag_t etc1;
		ext_flag_t etc1_srgb;
		ext_flag_t etc2_eac;
		ext_flag_t etc2_eac_srgb;
		ext_flag_t fxt1;
		ext_flag_t pvrtc2;
		ext_flag_t pvrtc;
		ext_flag_t pvrtc_srgb;
		ext_flag_t rgtc;
		ext_flag_t s3tc_dx1;
		ext_flag_t s3tc_dx5;
		ext_flag_t s3tc_srgb;
	} tex_format;
};

#define GL_VERSION_INT(mjr, mnr) (((mjr) << 8) + (mnr))

#undef GLANY_ATLEAST
#define GLANY_ATLEAST(mjr, mnr) (GL_VERSION_INT((mjr), (mnr)) <= GL_VERSION_INT(glext.version.major, glext.version.minor))

#undef GL_ATLEAST
#define GL_ATLEAST(mjr, mnr) (!glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#undef GLES_ATLEAST
#define GLES_ATLEAST(mjr, mnr) (glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#ifdef STATIC_GLES3
	#define HAVE_GL_FUNC(func) (&(func) != NULL)
#else
	#define HAVE_GL_FUNC(func) ((func) != NULL)
#endif
