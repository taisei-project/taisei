/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gles30.h"

#include "../glcommon/vtable.h"
#include "../glescommon/gles.h"
#include "../gl33/gl33.h"

#include <SDL3/SDL_video.h>

// NOTE: Actually WebGL
#ifdef STATIC_GLES3
	/*
	 * For reasons I don't even want to begin to imagine, using glBlitFramebuffer seems
	 * to trigger a Lovecraftian unholy abomination of a bug in Chromium that makes
	 * Taisei unplayable past the first stage. So we fake it with a draw command instead.
	 */
	#define BROKEN_GL_BLIT_FRAMEBUFFER 1

	/*
	 * SDL won't let us reuse the context across windows, even if they use the same cavas.
	 */
	#define WE_CAN_ONLY_HAVE_ONE_WINDOW 1
#else
	#define BROKEN_GL_BLIT_FRAMEBUFFER 0
	#define WE_CAN_ONLY_HAVE_ONE_WINDOW 0
#endif

#if WE_CAN_ONLY_HAVE_ONE_WINDOW

static SDL_Window *global_window;

static bool gles30_init(RendererBackend *backend, char *opts) {
	GLVT_OF(*backend).create_context = GLVT_OF(_r_backend_gl33).create_context;
	_r_backend_inherit(backend, &_r_backend_gl33);
	glcommon_setup_attributes(SDL_GL_CONTEXT_PROFILE_ES, 3, 0, 0);

	// It's not possible to reuse the context across windows in WebGL/Emscripten

	SDL_WindowFlags flags =
		SDL_WINDOW_HIDDEN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY |
		SDL_WINDOW_OPENGL |
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_FULLSCREEN;

	global_window = SDL_CreateWindow("", 1, 1, flags);

	if(!global_window) {
		log_sdl_error(LOG_ERROR, "SDL_CreateWindow");
		return NULL;
	}

	if(!GLVT_OF(*backend).init_context(backend, global_window)) {
		SDL_DestroyWindow(global_window);
		return false;
	}

	return true;
}

static SDL_Window *gles30_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	NOT_NULL(global_window);

	SDL_SetWindowTitle(global_window, title);
	SDL_SetWindowResizable(global_window, flags & SDL_WINDOW_RESIZABLE);
	SDL_SetWindowFullscreen(global_window, flags & SDL_WINDOW_FULLSCREEN);

	if(flags & SDL_WINDOW_HIDDEN) {
		SDL_HideWindow(global_window);
	} else {
		SDL_ShowWindow(global_window);
	}

	return global_window;
}

#else

static bool gles30_init(RendererBackend *backend, char *opts) {
	return gles_init(backend, 3, 0);
}

#endif

#if BROKEN_GL_BLIT_FRAMEBUFFER
#include "../gl33/gl33.h"
#include "fbcopy_fallback.h"

static bool gles30_init_context(RendererBackend *backend, SDL_Window *w) {
	if(!gles_init_context(backend, w)) {
		return false;
	}

	gles30_fbcopyfallback_init(backend);
	return true;
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

	#if WE_CAN_ONLY_HAVE_ONE_WINDOW
		.create_window = gles30_create_window,
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
