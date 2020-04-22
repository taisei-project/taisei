/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_renderer_glcommon_opengl_h
#define IGUARD_renderer_glcommon_opengl_h

#include "taisei.h"

#include <SDL.h>

#ifdef TAISEI_BUILDCONF_RENDERER_STATIC_GLES30
	#define STATIC_GLES3
#endif

#ifdef STATIC_GLES3
	#define GL_GLEXT_PROTOTYPES
	#include <GLES3/gl32.h>
	#include <GLES2/gl2ext.h>

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

	#define glClearDepth glClearDepthf
	#define glClearTexImage glClearTexImageEXT
#else
	#include <glad/gl.h>
	#define GL_FUNC(f) (glad_gl##f)

	#ifndef APIENTRY
		#define APIENTRY GLAD_API_PTR
	#endif
#endif

#include "assert.h"

// NOTE: The ability to query supported GLSL versions was added in GL 4.3,
// but it's not exposed by any extension. This is pretty silly.
#ifndef GL_NUM_SHADING_LANGUAGE_VERSIONS
	#define GL_NUM_SHADING_LANGUAGE_VERSIONS  0x82E9
#endif

#define TSGL_EXT_VENDORS \
	TSGL_EXT_VENDOR(AMD) \
	TSGL_EXT_VENDOR(ANGLE) \
	TSGL_EXT_VENDOR(APPLE) \
	TSGL_EXT_VENDOR(ARB) \
	TSGL_EXT_VENDOR(EXT) \
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
ext_flag_t glcommon_check_extension(const char *ext);
ext_flag_t glcommon_require_extension(const char *ext);
void glcommon_setup_attributes(SDL_GLprofile profile, uint major, uint minor, SDL_GLcontextFlag ctxflags);

#if defined(DEBUG) && defined(TAISEI_BUILDCONF_DEBUG_OPENGL)
	#define DEBUG_GL_DEFAULT 1
#else
	#define DEBUG_GL_DEFAULT 0
#endif

struct glext_s {
	struct {
		char major;
		char minor;
		bool is_es;
		bool is_ANGLE;
		bool is_webgl;
	} version;

	struct {
		uchar avoid_sampler_uniform_updates : 1;
	} issues;

	ext_flag_t base_instance;
	ext_flag_t clear_texture;
	ext_flag_t color_buffer_float;
	ext_flag_t debug_output;
	ext_flag_t depth_texture;
	ext_flag_t draw_buffers;
	ext_flag_t float_blend;
	ext_flag_t instanced_arrays;
	ext_flag_t pixel_buffer_object;
	ext_flag_t texture_filter_anisotropic;
	ext_flag_t texture_float_linear;
	ext_flag_t texture_half_float_linear;
	ext_flag_t texture_norm16;
	ext_flag_t texture_rg;
	ext_flag_t vertex_array_object;
	ext_flag_t viewport_array;
};

#define GL_VERSION_INT(mjr, mnr) (((mjr) << 8) + (mnr))

#undef GLANY_ATLEAST
#define GLANY_ATLEAST(mjr, mnr) (GL_VERSION_INT((mjr), (mnr)) <= GL_VERSION_INT(glext.version.major, glext.version.minor))

#undef GL_ATLEAST
#define GL_ATLEAST(mjr, mnr) (!glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#undef GLES_ATLEAST
#define GLES_ATLEAST(mjr, mnr) (glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#ifdef STATIC_GLESS
	#define HAVE_GL_FUNC(func) (&(func) != NULL)
#else
	#define HAVE_GL_FUNC(func) ((func) != NULL)
#endif

#endif // IGUARD_renderer_glcommon_opengl_h
