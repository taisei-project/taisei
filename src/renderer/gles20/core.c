/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "core.h"
#include "../gl33/core.h"
#include "../glcommon/debug.h"
#include "../glcommon/vtable.h"

static void gles20_init_context(SDL_Window *w) {
	GLVT_OF(_r_backend_gl33).init_context(w);
	// glcommon_require_extension("GL_OES_depth_texture");
}

static void gles_init(RendererBackend *gles_backend, int major, int minor) {
	// omg inheritance
	RendererFuncs original_funcs;
	memcpy(&original_funcs, &gles_backend->funcs, sizeof(original_funcs));
	memcpy(&gles_backend->funcs, &_r_backend_gl33.funcs, sizeof(gles_backend->funcs));
	gles_backend->funcs.init = original_funcs.init;
	gles_backend->funcs.screenshot = original_funcs.screenshot;

	SDL_GLcontextFlag ctxflags = 0;

	if(glcommon_debug_requested()) {
		ctxflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, ctxflags);

	glcommon_load_library();
}

static void gles20_init(void) {
	gles_init(&_r_backend_gles20, 2, 0);
}

static void gles30_init(void) {
	gles_init(&_r_backend_gles30, 3, 0);
}

GLTextureTypeInfo* gles20_texture_type_info(TextureType type) {
	static GLTextureFormatTuple color_formats[] = {
		{ GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8     },
		// { GL_RED,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16    },
		// { GL_RED,  GL_UNSIGNED_INT,   PIXMAP_FORMAT_R32    },

		{ GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8    },
		// { GL_RG,   GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RG16   },
		// { GL_RG,   GL_UNSIGNED_INT,   PIXMAP_FORMAT_RG32   },

		{ GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8   },
		// { GL_RGB,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGB16  },
		// { GL_RGB,  GL_UNSIGNED_INT,   PIXMAP_FORMAT_RGB32  },

		{ GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8  },
		// { GL_RGBA, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGBA16 },
		// { GL_RGBA, GL_UNSIGNED_INT,   PIXMAP_FORMAT_RGBA32 },

		{ 0 }
	};

	// XXX: I'm not sure about this. Perhaps it's better to not support depth texture uploading at all?
	static GLTextureFormatTuple depth_formats[] = {
		// { GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8 },
		{ GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 },
		// { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,   PIXMAP_FORMAT_R32 },

		{ 0 }
	};

	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R]         = { GL_RED,  1, color_formats, { GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8    } },
		[TEX_TYPE_RG]        = { GL_RG,   2, color_formats, { GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8   } },
		[TEX_TYPE_RGB]       = { GL_RGB,  3, color_formats, { GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8  } },
		[TEX_TYPE_RGBA]      = { GL_RGBA, 4, color_formats, { GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8 } },
		[TEX_TYPE_DEPTH]     = { GL_DEPTH_COMPONENT, 2, depth_formats, { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 } },
	};

	assert((uint)type < sizeof(map)/sizeof(*map));
	return map + type;
}

static bool gles20_screenshot(Pixmap *out) {
	IntRect vp;
	r_framebuffer_viewport_current(NULL, &vp);
	out->width = vp.w;
	out->height = vp.h;
	out->format = PIXMAP_FORMAT_RGBA8;
	out->origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	out->data.untyped = pixmap_alloc_buffer_for_copy(out);
	glReadPixels(vp.x, vp.y, vp.w, vp.h, GL_RGBA, GL_UNSIGNED_BYTE, out->data.untyped);
	return true;
}

RendererBackend _r_backend_gles20 = {
	.name = "gles20",
	.funcs = {
		.init = gles20_init,
		.screenshot = gles20_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gles20_texture_type_info,
			.init_context = gles20_init_context,
		}
	},
};

RendererBackend _r_backend_gles30 = {
	.name = "gles30",
	.funcs = {
		.init = gles30_init,
		.screenshot = gles20_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gles20_texture_type_info,
			.init_context = gles20_init_context,
		}
	},
};
