/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <assert.h>
#include "global.h"

#define TAISEIGL_NO_EXT_ABSTRACTION
#include "taiseigl.h"
#undef TAISEIGL_NO_EXT_ABSTRACTION

struct glext_s glext;

#ifndef LINK_TO_LIBGL
#define GLDEF(glname,tsname,typename) typename tsname;
GLDEFS
#undef GLDEF
#endif

typedef void (*tsglproc_ptr)();

#ifndef LINK_TO_LIBGL
static tsglproc_ptr get_proc_address(const char *name) {
	void *addr = SDL_GL_GetProcAddress(name);

	if(!addr) {
		log_warn("SDL_GL_GetProcAddress(\"%s\") failed: %s", name, SDL_GetError());
	}

	// shut up a stupid warning about conversion between data pointers and function pointers
	// yes, such a conversion is not allowed by the standard, but when several major APIs
	// (POSIX and SDL to name a few) require casting void* to a function pointer, that says something.
	return *(tsglproc_ptr*)&addr;
}
#endif

static void get_gl_version(char *major, char *minor) {
	// the glGetIntegerv way only works in >=3.0 contexts, so...

	const char *vstr = (const char*)glGetString(GL_VERSION);

	if(!vstr) {
		*major = 1;
		*minor = 1;
		return;
	}

	const char *dot = strchr(vstr, '.');

	*major = atoi(vstr);
	*minor = atoi(dot+1);
}

void check_gl_extensions(void) {
	get_gl_version(&glext.version.major, &glext.version.minor);

	const char *glslv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	if(!glslv) {
		glslv = "None";
	}

	log_info("OpenGL version: %s", (const char*)glGetString(GL_VERSION));
	log_info("OpenGL vendor: %s", (const char*)glGetString(GL_VENDOR));
	log_info("OpenGL renderer: %s", (const char*)glGetString(GL_RENDERER));
	log_info("GLSL version: %s", glslv);

	glext.draw_instanced = false;
	glext.DrawArraysInstanced = NULL;

	glext.EXT_draw_instanced = SDL_GL_ExtensionSupported("GL_EXT_draw_instanced");
	glext.ARB_draw_instanced = SDL_GL_ExtensionSupported("GL_ARB_draw_instanced");

	glext.draw_instanced = glext.EXT_draw_instanced;

	if(glext.draw_instanced) {
		glext.DrawArraysInstanced = tsglDrawArraysInstancedEXT;
		if(!glext.DrawArraysInstanced) {
			glext.draw_instanced = false;
		}
	}

	if(!glext.draw_instanced) {
		glext.draw_instanced = glext.ARB_draw_instanced;

		if(glext.draw_instanced) {
			glext.DrawArraysInstanced = tsglDrawArraysInstancedARB;
			if(!glext.DrawArraysInstanced) {
				glext.draw_instanced = false;
			}
		}
	}

	if(!glext.draw_instanced) {
		log_warn(
			"glDrawArraysInstanced is not supported. "
			"Your video driver is probably bad, or very old, or both. "
			"Expect terrible performance."
		);
	}
}

void load_gl_library(void) {
#ifndef LINK_TO_LIBGL
	char *lib = getenv("TAISEI_LIBGL");

	if(lib && !*lib) {
		lib = NULL;
	}

	if(SDL_GL_LoadLibrary(lib) < 0) {
		log_fatal("SDL_GL_LoadLibrary() failed: %s", SDL_GetError());
	}
#endif
}

void unload_gl_library(void) {
#ifndef LINK_TO_LIBGL
	SDL_GL_UnloadLibrary();
#endif
}

void load_gl_functions(void) {
#ifndef LINK_TO_LIBGL
#define GLDEF(glname,tsname,typename) tsname = (typename)get_proc_address(#glname);
	GLDEFS
#undef GLDEF
#endif
}

void gluPerspective(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
    GLdouble fW, fH;
    fH = tan(fovY / 360 * M_PI) * zNear;
    fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}
