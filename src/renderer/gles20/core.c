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

	glcommon_setup_attributes(SDL_GL_CONTEXT_PROFILE_ES, major, minor, 0);
	glcommon_load_library();
}

static void gles20_init(void) {
	gles_init(&_r_backend_gles20, 2, 0);
}

static void gles30_init(void) {
	gles_init(&_r_backend_gles30, 3, 0);
}

GLTextureTypeInfo* gles20_texture_type_info(TextureType type) {
	#define FMT_R     { GL_RED,             GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8    }
	#define FMT_RG    { GL_RG,              GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8   }
	#define FMT_RGB   { GL_RGB,             GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8  }
	#define FMT_RGBA  { GL_RGBA,            GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8 }
	#define FMT_DEPTH { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16   }

	static GLTextureFormatTuple formats_r[]     = { FMT_R,     { 0 } };
	static GLTextureFormatTuple formats_rg[]    = { FMT_RG,    { 0 } };
	static GLTextureFormatTuple formats_rgb[]   = { FMT_RGB,   { 0 } };
	static GLTextureFormatTuple formats_rgba[]  = { FMT_RGBA,  { 0 } };
	static GLTextureFormatTuple formats_depth[] = { FMT_DEPTH, { 0 } };

	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R_8]            = { GL_RGBA, formats_r,    FMT_R    },
		[TEX_TYPE_RG_8]           = { GL_RGBA, formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_8]          = { GL_RGBA, formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_8]         = { GL_RGBA, formats_rgba, FMT_RGBA },

		[TEX_TYPE_R_16]           = { GL_RGBA, formats_r,    FMT_R    },
		[TEX_TYPE_RG_16]          = { GL_RGBA, formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_16]         = { GL_RGBA, formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_16]        = { GL_RGBA, formats_rgba, FMT_RGBA },

		[TEX_TYPE_R_32_FLOAT]     = { GL_RGBA, formats_r,    FMT_R    },
		[TEX_TYPE_RG_32_FLOAT]    = { GL_RGBA, formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_32_FLOAT]   = { GL_RGBA, formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_32_FLOAT]  = { GL_RGBA, formats_rgba, FMT_RGBA },

		// WARNING: ANGLE bug(?): texture binding fails if a sized format is used here.
		[TEX_TYPE_DEPTH_8]        = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
		[TEX_TYPE_DEPTH_16]       = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
		[TEX_TYPE_DEPTH_32_FLOAT] = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
	};

	#undef FMT_R
	#undef FMT_RG
	#undef FMT_RGB
	#undef FMT_RGBA
	#undef FMT_DEPTH

	assert((uint)type < sizeof(map)/sizeof(*map));
	return map + type;
}

GLTextureTypeInfo* gles30_texture_type_info(TextureType type) {
	#define FMT_R     { GL_RED,             GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8    }
	#define FMT_RG    { GL_RG,              GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8   }
	#define FMT_RGB   { GL_RGB,             GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8  }
	#define FMT_RGBA  { GL_RGBA,            GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8 }
	#define FMT_DEPTH { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16   }

	static GLTextureFormatTuple formats_r[]     = { FMT_R,     { 0 } };
	static GLTextureFormatTuple formats_rg[]    = { FMT_RG,    { 0 } };
	static GLTextureFormatTuple formats_rgb[]   = { FMT_RGB,   { 0 } };
	static GLTextureFormatTuple formats_rgba[]  = { FMT_RGBA,  { 0 } };
	static GLTextureFormatTuple formats_depth[] = { FMT_DEPTH, { 0 } };

	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R_8]            = { GL_R8,    formats_r,    FMT_R    },
		[TEX_TYPE_RG_8]           = { GL_RG8,   formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_8]          = { GL_RGB8,  formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_8]         = { GL_RGBA8, formats_rgba, FMT_RGBA },

		[TEX_TYPE_R_16]           = { GL_R8,    formats_r,    FMT_R    },
		[TEX_TYPE_RG_16]          = { GL_RG8,   formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_16]         = { GL_RGB8,  formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_16]        = { GL_RGBA8, formats_rgba, FMT_RGBA },

		[TEX_TYPE_R_32_FLOAT]     = { GL_R8,    formats_r,    FMT_R    },
		[TEX_TYPE_RG_32_FLOAT]    = { GL_RG8,   formats_rg,   FMT_RG   },
		[TEX_TYPE_RGB_32_FLOAT]   = { GL_RGB8,  formats_rgb,  FMT_RGB  },
		[TEX_TYPE_RGBA_32_FLOAT]  = { GL_RGBA8, formats_rgba, FMT_RGBA },

		// WARNING: ANGLE bug(?): texture binding fails if a sized format is used here.
		[TEX_TYPE_DEPTH_8]        = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
		[TEX_TYPE_DEPTH_16]       = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
		[TEX_TYPE_DEPTH_32_FLOAT] = { GL_DEPTH_COMPONENT, formats_depth, FMT_DEPTH },
	};

	#undef FMT_R
	#undef FMT_RG
	#undef FMT_RGB
	#undef FMT_RGBA
	#undef FMT_DEPTH

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
			.texture_type_info = gles30_texture_type_info,
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
			.texture_type_info = gles30_texture_type_info,
			.init_context = gles20_init_context,
		}
	},
};
