/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"

#define TAISEIGL_NO_EXT_ABSTRACTION
#include "opengl.h"
#undef TAISEIGL_NO_EXT_ABSTRACTION

struct glext_s glext;

#ifndef LINK_TO_LIBGL
#define GLDEF(glname,tsname,typename) typename tsname;
GLDEFS
#undef GLDEF
#endif

typedef void (*tsglproc_ptr)(void);

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

static bool extension_supported(const char *ext) {
	const char *overrides = getenv("TAISEI_GL_EXT_OVERRIDES");

	if(overrides) {
		char buf[strlen(overrides)+1], *save, *arg, *e;
		strcpy(buf, overrides);
		arg = buf;

		while((e = strtok_r(arg, " ", &save))) {
			bool r = true;

			if(*e == '-') {
				++e;
				r = false;
			}

			if(!strcmp(e, ext)) {
				return r;
			}

			arg = NULL;
		}
	}

	return SDL_GL_ExtensionSupported(ext);
}

static void check_glext_debug_output(void) {
	if((glext.debug_output = (
		(glext.KHR_debug = extension_supported("GL_KHR_debug")) &&
		(glext.DebugMessageCallback = tsglDebugMessageCallback) &&
		(glext.DebugMessageControl = tsglDebugMessageControl)
	))) {
		log_debug("Using GL_KHR_debug");
		return;
	}

	if((glext.debug_output = (
		(glext.ARB_debug_output = extension_supported("GL_ARB_debug_output")) &&
		(glext.DebugMessageCallback = tsglDebugMessageCallbackARB) &&
		(glext.DebugMessageControl = tsglDebugMessageControlARB)
	))) {
		log_debug("Using GL_ARB_debug_output");
		return;
	}
}

void check_gl_extensions(void) {
	memset(&glext, 0, sizeof(glext));
	get_gl_version(&glext.version.major, &glext.version.minor);

	const char *glslv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	if(!glslv) {
		glslv = "None";
	}

	log_info("OpenGL version: %s", (const char*)glGetString(GL_VERSION));
	log_info("OpenGL vendor: %s", (const char*)glGetString(GL_VENDOR));
	log_info("OpenGL renderer: %s", (const char*)glGetString(GL_RENDERER));
	log_info("GLSL version: %s", glslv);

	// XXX: this is the legacy way, maybe we shouldn't try this first
	const char *exts = (const char*)glGetString(GL_EXTENSIONS);

	if(exts) {
		log_debug("Supported extensions: %s", exts);
	} else {
		void *buf;
		SDL_RWops *writer = SDL_RWAutoBuffer(&buf, 1024);
		GLint num_extensions;

		SDL_RWprintf(writer, "Supported extensions:");
		glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

		for(int i = 0; i < num_extensions; ++i) {
			SDL_RWprintf(writer, " %s", (const char*)glGetStringi(GL_EXTENSIONS, i));
		}

		SDL_WriteU8(writer, 0);
		log_debug("%s", (char*)buf);
		SDL_RWclose(writer);
	}

	check_glext_debug_output();
	glext.ARB_base_instance = (
		extension_supported("GL_ARB_base_instance")
		&& tsglDrawArraysInstancedBaseInstance
		&& tsglDrawElementsInstancedBaseInstance
	);
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
