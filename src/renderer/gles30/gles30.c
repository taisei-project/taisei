/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gles30.h"

#include "../glcommon/vtable.h"
#include "../glescommon/gles.h"

// NOTE: Actually WebGL
#ifdef STATIC_GLES3
	/*
	 * For reasons I don't even want to begin to imagine, using glBlitFramebuffer seems
	 * to trigger a Lovecraftian unholy abomination of a bug in Chromium that makes
	 * Taisei unplayable past the first stage. So we fake it with a draw command instead.
	 */
	#define BROKEN_GL_BLIT_FRAMEBUFFER 1
#else
	#define BROKEN_GL_BLIT_FRAMEBUFFER 0
#endif

static void gles30_init(void) {
	gles_init(&_r_backend_gles30, 3, 0);
}

#if BROKEN_GL_BLIT_FRAMEBUFFER
#include "../gl33/gl33.h"
#include "fbcopy_fallback.h"

static void gles30_init_context(SDL_Window *w) {
	gles_init_context(w);
	gles30_fbcopyfallback_init();
}

static void gles30_shutdown(void) {
	gles30_fbcopyfallback_shutdown();
	_r_backend_gl33.funcs.shutdown();
}
#endif

RendererBackend _r_backend_gles30 = {
	.name = "gles30",
	.funcs = {
		.init = gles30_init,
		.texture_dump = gles_texture_dump,

	#if BROKEN_GL_BLIT_FRAMEBUFFER
		.shutdown = gles30_shutdown,
		.framebuffer_copy = gles30_fbcopyfallback_framebuffer_copy,
	#endif
	},
	.custom = &(GLBackendData) {
		.vtable = {
		#if BROKEN_GL_BLIT_FRAMEBUFFER
			.init_context = gles30_init_context,
		#else
			.init_context = gles_init_context,
		#endif
		}
	},
};
