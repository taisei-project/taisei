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
	memcpy(&gles_backend->res_handlers, &_r_backend_gl33.res_handlers, sizeof(gles_backend->res_handlers));
	gles_backend->funcs.init = original_funcs.init;

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
	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R]       = { GL_RED,             GL_RED,             GL_UNSIGNED_BYTE,  1 },
		[TEX_TYPE_RG]      = { GL_RG,              GL_RG,              GL_UNSIGNED_BYTE,  2 },
		[TEX_TYPE_RGB]     = { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE,  4 },
		[TEX_TYPE_RGBA]    = { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE,  4 },
		[TEX_TYPE_DEPTH]   = { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 2 },
	};

	assert((uint)type < sizeof(map)/sizeof(*map));
	return map + type;
}

RendererBackend _r_backend_gles20 = {
	.name = "gles20",
	.funcs = {
		.init = gles20_init,
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
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gles20_texture_type_info,
			.init_context = gles20_init_context,
		}
	},
};
