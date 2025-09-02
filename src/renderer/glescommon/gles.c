/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gles.h"

#include "../common/backend.h"
#include "../gl33/gl33.h"
#include "../glcommon/vtable.h"
#include "util/env.h"

#define SDL_USE_BUILTIN_OPENGL_DEFINITIONS
#include <SDL3/SDL_egl.h>

#ifdef _WIN32
	// Enable WebGL compatibility mode on Windows, because cubemaps are broken in the D3D11 backend
	// except in WebGL mode for some reason.
	#define ANGLE_WEBGL_DEFAULT true
#else
	#define ANGLE_WEBGL_DEFAULT false
#endif

#ifndef EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE
#define EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE 0x33ac
#endif

#ifndef EGL_EXTENSIONS_ENABLED_ANGLE
#define EGL_EXTENSIONS_ENABLED_ANGLE 0x345f
#endif

attr_unused
static SDL_EGLint *gles_angle_egl_context_attrib_callback(
	void *userdata, SDL_EGLDisplay display, SDL_EGLConfig config
) {
	SDL_EGLint a[] = {
		EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE, EGL_TRUE,
		EGL_EXTENSIONS_ENABLED_ANGLE, EGL_TRUE,
		EGL_NONE,
	};
	return memcpy(SDL_malloc(sizeof(a)), a, sizeof(a));
}

void gles_init(RendererBackend *gles_backend, int major, int minor) {
	GLVT_OF(*gles_backend).create_context = GLVT_OF(_r_backend_gl33).create_context;

#ifdef TAISEI_BUILDCONF_HAVE_ANGLE
	// Load ANGLE by default by setting up some SDL-specific environment vars.
	// These are not overwritten if they are already set in the environment, so
	// you can still override this behavior as usual.

	#ifdef TAISEI_BUILDCONF_RELOCATABLE_INSTALL
	// In a relocatable build, paths are relative to SDL_GetBasePath
	const char *basepath = SDL_GetBasePath();
	char buf[strlen(basepath) + sizeof(TAISEI_BUILDCONF_ANGLE_GLES_PATH) + sizeof(TAISEI_BUILDCONF_ANGLE_EGL_PATH)];

	snprintf(buf, sizeof(buf), "%s%s", basepath, TAISEI_BUILDCONF_ANGLE_GLES_PATH);
	env_set("SDL_VIDEO_GL_DRIVER", buf, false);

	snprintf(buf, sizeof(buf), "%s%s", basepath, TAISEI_BUILDCONF_ANGLE_EGL_PATH);
	env_set("SDL_VIDEO_EGL_DRIVER", buf, false);
	#else
	// Static absolute paths
	env_set("SDL_VIDEO_GL_DRIVER", TAISEI_BUILDCONF_ANGLE_GLES_PATH, false);
	env_set("SDL_VIDEO_EGL_DRIVER", TAISEI_BUILDCONF_ANGLE_EGL_PATH, false);
	#endif

	env_set("SDL_OPENGL_ES_DRIVER", 1, false);
#endif // TAISEI_BUILDCONF_HAVE_ANGLE

	_r_backend_inherit(gles_backend, &_r_backend_gl33);
	glcommon_setup_attributes(SDL_GL_CONTEXT_PROFILE_ES, major, minor, 0);

#ifdef TAISEI_BUILDCONF_HAVE_ANGLE
	if(env_get("TAISEI_ANGLE_WEBGL", ANGLE_WEBGL_DEFAULT)) {
		SDL_EGL_SetAttributeCallbacks(NULL, NULL, gles_angle_egl_context_attrib_callback, NULL);
	}
#endif

	glcommon_load_library();
}

void gles_init_context(SDL_Window *w) {
	GLVT_OF(_r_backend_gl33).init_context(w);
}

bool gles_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst) {
	// No glGetTexImage in GLES
	// TODO maybe set up a transient framebuffer to read from?
	log_warn("FIXME: not implemented");
	return false;
}
