/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>

#include "glad/glad.h"
#include "assert.h"

// NOTE: The ability to query supported GLSL versions was added in GL 4.3,
// but it's not exposed by any extension. This is pretty silly.
#ifndef GL_NUM_SHADING_LANGUAGE_VERSIONS
	#define GL_NUM_SHADING_LANGUAGE_VERSIONS  0x82E9
#endif

#define TSGL_EXT_VENDORS \
	TSGL_EXT_VENDOR(NATIVE) \
	TSGL_EXT_VENDOR(KHR) \
	TSGL_EXT_VENDOR(ARB) \
	TSGL_EXT_VENDOR(EXT) \
	TSGL_EXT_VENDOR(OES) \
	TSGL_EXT_VENDOR(AMD) \
	TSGL_EXT_VENDOR(NV) \
	TSGL_EXT_VENDOR(ANGLE) \
	TSGL_EXT_VENDOR(MESA) \

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
void glcommon_check_extensions(void);
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
	} version;

	ext_flag_t debug_output;
	ext_flag_t instanced_arrays;
	ext_flag_t base_instance;
	ext_flag_t pixel_buffer_object;
	ext_flag_t depth_texture;
	ext_flag_t draw_buffers;
	ext_flag_t texture_filter_anisotropic;
	ext_flag_t clear_texture;

	//
	// debug_output
	//

	PFNGLDEBUGMESSAGECONTROLKHRPROC DebugMessageControl;
	#undef glDebugMessageControl
	#define glDebugMessageControl (glext.DebugMessageControl)

	PFNGLDEBUGMESSAGECALLBACKKHRPROC DebugMessageCallback;
	#undef glDebugMessageCallback
	#define glDebugMessageCallback (glext.DebugMessageCallback)

	PFNGLOBJECTLABELPROC ObjectLabel;
	#undef glObjectLabel
	#define glObjectLabel (glext.ObjectLabel)

	// instanced_arrays

	PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced;
	#undef glDrawArraysInstanced
	#define glDrawArraysInstanced (glext.DrawArraysInstanced)

	PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced;
	#undef glDrawElementsInstanced
	#define glDrawElementsInstanced (glext.DrawElementsInstanced)

	PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor;
	#undef glVertexAttribDivisor
	#define glVertexAttribDivisor (glext.VertexAttribDivisor)

	//
	// base_instance
	//

	PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC DrawArraysInstancedBaseInstance;
	#undef glDrawArraysInstancedBaseInstance
	#define glDrawArraysInstancedBaseInstance (glext.DrawArraysInstancedBaseInstance)

	PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC DrawElementsInstancedBaseInstance;
	#undef glDrawElementsInstancedBaseInstance
	#define glDrawElementsInstancedBaseInstance (glext.DrawElementsInstancedBaseInstance)

	//
	// draw_buffers
	//

	PFNGLDRAWBUFFERSPROC DrawBuffers;
	#undef glDrawBuffers
	#define glDrawBuffers (glext.DrawBuffers)

	//
	// clear_texture
	// NOTE: no need for indirection here; the entrypoint names are the same.
	//
	/*
	PFNGLCLEARTEXIMAGEPROC ClearTexImage;
	#undef glClearTexImage
	#define glClearTexImage (glext.ClearTexImage)

	PFNGLCLEARTEXSUBIMAGEPROC ClearTexSubImage;
	#undef glClearTexSubImage
	#define glClearTexSubImage (glext.ClearTexSubImage)
	*/
};

#undef GLANY_ATLEAST
#define GLANY_ATLEAST(mjr, mnr) (glext.version.major >= (mjr) && glext.version.minor >= (mnr))

#undef GL_ATLEAST
#define GL_ATLEAST(mjr, mnr) (!glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#undef GLES_ATLEAST
#define GLES_ATLEAST(mjr, mnr) (glext.version.is_es && GLANY_ATLEAST(mjr, mnr))
