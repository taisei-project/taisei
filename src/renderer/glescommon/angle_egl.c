/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "angle_egl.h"
#include "../glcommon/debug.h"
#include "glad/egl.h"
#include "util.h"

/*
 * All this garbage here serves one purpose: create a WebGL-compatible ANGLE context.
 *
 * To do this we should only need to pass one additional attribute to eglCreateContext.
 *
 * Unfortunately, SDL2's EGL code is totally opaque to the application, so that can't be done in a
 * sane way. So we have to sidestep SDL a bit and create a context manually, relying on some
 * implementation details.
 *
 * SDL3 seems to have fixed this problem with some new EGL-related APIs, thankfully.
 *
 * This code is a bad hack; please don't copy this if you're looking for a proper EGL example.
 */

// EGL_ANGLE_create_context_webgl_compatibility
#define EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE 0x33AC

// EGL_ANGLE_create_context_extensions_enabled
#define EGL_EXTENSIONS_ENABLED_ANGLE 0x345F

static GLADapiproc egl_load_function(void *libegl, const char *name) {
	return SDL_LoadFunction(libegl, name);
}

static const char *egl_error_string(EGLint error) {
	#define E(e) \
		case e: return #e;

	switch(error) {
		E(EGL_SUCCESS)
		E(EGL_NOT_INITIALIZED)
		E(EGL_BAD_ACCESS)
		E(EGL_BAD_ALLOC)
		E(EGL_BAD_ATTRIBUTE)
		E(EGL_BAD_CONTEXT)
		E(EGL_BAD_CONFIG)
		E(EGL_BAD_CURRENT_SURFACE)
		E(EGL_BAD_DISPLAY)
		E(EGL_BAD_SURFACE)
		E(EGL_BAD_MATCH)
		E(EGL_BAD_PARAMETER)
		E(EGL_BAD_NATIVE_PIXMAP)
		E(EGL_BAD_NATIVE_WINDOW)
		E(EGL_CONTEXT_LOST)
	}

	#undef E

	UNREACHABLE;
}

static const char *egl_get_error_string(void) {
	return egl_error_string(eglGetError());
}

static EGLConfig get_egl_config(EGLDisplay display, int major_version) {
	EGLint attribs[64];
	EGLConfig configs[128];
	int i = 0;

	int colorbits = 8;

	attribs[i++] = EGL_RED_SIZE;
	attribs[i++] = colorbits;
	attribs[i++] = EGL_GREEN_SIZE;
	attribs[i++] = colorbits;
	attribs[i++] = EGL_BLUE_SIZE;
	attribs[i++] = colorbits;
	attribs[i++] = EGL_ALPHA_SIZE;
	attribs[i++] = 0;
	attribs[i++] = EGL_DEPTH_SIZE;
	attribs[i++] = 0;
	attribs[i++] = EGL_CONFIG_CAVEAT;
	attribs[i++] = EGL_NONE;
	attribs[i++] = EGL_RENDERABLE_TYPE;
	attribs[i++] = (major_version > 2) ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT;
	attribs[i++] = EGL_NONE;

	EGLint found_configs;

	if(!eglChooseConfig(display, attribs, configs, ARRAY_SIZE(configs), &found_configs)) {
		log_fatal("eglChooseConfig failed: %s", egl_get_error_string());
	}

	if(!found_configs) {
		log_fatal("No EGL configs found");
	}

	log_debug("Found %i configs", found_configs);

	int choosen_config = 0;

	for(int i = 0; i < found_configs; ++i) {
		EGLint r, g, b;
		eglGetConfigAttrib(display, configs[i], EGL_RED_SIZE, &r);
		eglGetConfigAttrib(display, configs[i], EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(display, configs[i], EGL_BLUE_SIZE, &b);

		if(r == colorbits && g == colorbits && b == colorbits) {
			// just grab the first one that looks like it'll work
			choosen_config = i;
			break;
		}
	}

	return configs[choosen_config];
}

static EGLDisplay egl_display_from_sdl(SDL_Window *window) {
	// this is dumb as hell, but it's the only way
	EGLContext ctx = SDL_GL_CreateContext(window);

	if(!ctx) {
		log_sdl_error(LOG_FATAL, "SDL_GL_CreateContext");
	}

	EGLDisplay display = eglGetCurrentDisplay();
	assert(display != EGL_NO_DISPLAY);

	SDL_GL_DeleteContext(ctx);
	return display;
}

SDL_GLContext gles_create_context_angle(SDL_Window *window, int major, int minor, bool webgl) {
	void *libegl = SDL_LoadObject(NOT_NULL(env_get("SDL_VIDEO_EGL_DRIVER", NULL)));

	if(!libegl) {
		log_sdl_error(LOG_FATAL, "SDL_LoadObject");
	}

	gladLoadEGLUserPtr(EGL_NO_DISPLAY, egl_load_function, libegl);
	EGLDisplay display = egl_display_from_sdl(window);
	gladLoadEGLUserPtr(display, egl_load_function, libegl);

	eglBindAPI(EGL_OPENGL_ES_API);
	EGLConfig config = get_egl_config(display, major);

	EGLint attribs[64];
	int i = 0;

	attribs[i++] = EGL_CONTEXT_MAJOR_VERSION;
	attribs[i++] = major;

	attribs[i++] = EGL_CONTEXT_MINOR_VERSION;
	attribs[i++] = minor;

	attribs[i++] = EGL_CONTEXT_OPENGL_DEBUG;
	attribs[i++] = glcommon_debug_requested();

	attribs[i++] = EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE;
	attribs[i++] = webgl;

	attribs[i++] = EGL_EXTENSIONS_ENABLED_ANGLE;
	attribs[i++] = EGL_TRUE;

	attribs[i++] = EGL_NONE;

	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, attribs);

	if(context == EGL_NO_CONTEXT) {
		log_fatal("eglCreateContext failed: %s", egl_get_error_string());
	}

	if(SDL_GL_MakeCurrent(window, context)) {
		log_sdl_error(LOG_FATAL, "SDL_GL_MakeCurrent");
	}

	assert(SDL_GL_GetCurrentContext() == context);

	SDL_UnloadObject(libegl);

	return context;
}

