/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"

#include "opengl.h"

struct glext_s glext;

#ifndef LINK_TO_LIBGL
#define GLDEF(glname,tsname,typename) typename tsname = NULL;
GLDEFS
#undef GLDEF
#endif

typedef void (*tsglproc_ptr)(void);

static const char *const ext_vendor_table[] = {
	#define TSGL_EXT_VENDOR(v) [_TSGL_EXTVNUM_##v] = #v,
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR

	NULL,
};

static ext_flag_t glcommon_ext_flag(const char *ext) {
	assert(ext != NULL);
	ext = strchr(ext, '_');

	if(ext == NULL) {
		log_fatal("Bad extension string: %s", ext);
	}

	const char *sep = strchr(++ext, '_');

	if(sep == NULL) {
		log_fatal("Bad extension string: %s", ext);
	}

	char vendor[sep - ext + 1];
	strlcpy(vendor, ext, sizeof(vendor));

	for(const char *const *p = ext_vendor_table; *p; ++p) {
		if(!strcmp(*p, vendor)) {
			return 1 << (p - ext_vendor_table);
		}
	}

	log_fatal("Unknown vendor '%s' in extension string %s", vendor, ext);
}

#ifndef LINK_TO_LIBGL
static tsglproc_ptr glcommon_proc_address(const char *name) {
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

static void glcommon_gl_version(char *major, char *minor) {
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

ext_flag_t glcommon_check_extension(const char *ext) {
	const char *overrides = env_get("TAISEI_GL_EXT_OVERRIDES", "");
	ext_flag_t flag = glcommon_ext_flag(ext);

	if(*overrides) {
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
				return r ? flag : 0;
			}

			arg = NULL;
		}
	}

	return SDL_GL_ExtensionSupported(ext) ? flag : 0;
}

ext_flag_t glcommon_require_extension(const char *ext) {
	ext_flag_t val = glcommon_check_extension(ext);

	if(!val) {
		if(env_get("TAISEI_GL_REQUIRE_EXTENSION_FATAL", 0)) {
			log_fatal("Required extension %s is not available", ext);
		}

		log_warn("Required extension %s is not available, expect crashes or rendering errors", ext);
	}

	return val;
}

static void glcommon_ext_debug_output(void) {
	if(
		GL_ATLEAST(4, 3)
		&& (glext.DebugMessageCallback = tsglDebugMessageCallback)
		&& (glext.DebugMessageControl = tsglDebugMessageControl)
		&& (glext.ObjectLabel = tsglObjectLabel)
	) {
		glext.debug_output = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if(glext.version.is_es) {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = tsglDebugMessageCallbackKHR)
			&& (glext.DebugMessageControl = tsglDebugMessageControlKHR)
			&& (glext.ObjectLabel = tsglObjectLabelKHR)
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	} else {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = tsglDebugMessageCallback)
			&& (glext.DebugMessageControl = tsglDebugMessageControl)
			&& (glext.ObjectLabel = tsglObjectLabel)
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	}

	if((glext.debug_output = glcommon_check_extension("GL_ARB_debug_output"))
		&& (glext.DebugMessageCallback = tsglDebugMessageCallbackARB)
		&& (glext.DebugMessageControl = tsglDebugMessageControlARB)
	) {
		log_info("Using GL_ARB_debug_output");
		return;
	}

	glext.debug_output = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_base_instance(void) {
	if(
		GL_ATLEAST(4, 2)
		&& (glext.DrawArraysInstancedBaseInstance = tsglDrawArraysInstancedBaseInstance)
		&& (glext.DrawElementsInstancedBaseInstance = tsglDrawElementsInstancedBaseInstance)
	) {
		glext.base_instance = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.base_instance = glcommon_check_extension("GL_ARB_base_instance"))
		&& (glext.DrawArraysInstancedBaseInstance = tsglDrawArraysInstancedBaseInstance)
		&& (glext.DrawElementsInstancedBaseInstance = tsglDrawElementsInstancedBaseInstance)
	) {
		log_info("Using GL_ARB_base_instance");
		return;
	}

	if((glext.base_instance = glcommon_check_extension("GL_EXT_base_instance"))
		&& (glext.DrawArraysInstancedBaseInstance = tsglDrawArraysInstancedBaseInstanceEXT)
		&& (glext.DrawElementsInstancedBaseInstance = tsglDrawElementsInstancedBaseInstanceEXT)
	) {
		log_info("Using GL_EXT_base_instance");
		return;
	}

	glext.base_instance = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_pixel_buffer_object(void) {
	// TODO: verify that these requirements are correct
	if(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0)) {
		glext.pixel_buffer_object = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	const char *exts[] = {
		"GL_ARB_pixel_buffer_object",
		"GL_EXT_pixel_buffer_object",
		"GL_NV_pixel_buffer_object",
		NULL
	};

	for(const char **p = exts; *p; ++p) {
		if((glext.pixel_buffer_object = glcommon_check_extension(*p))) {
			log_info("Using %s", *p);
			return;
		}
	}

	glext.pixel_buffer_object = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_depth_texture(void) {
	// TODO: detect this for core OpenGL properly
	if(!glext.version.is_es || GLES_ATLEAST(3, 0)) {
		glext.depth_texture = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	const char *exts[] = {
		"GL_OES_depth_texture",
		"GL_ANGLE_depth_texture",
		NULL
	};

	for(const char **p = exts; *p; ++p) {
		if((glext.pixel_buffer_object = glcommon_check_extension(*p))) {
			log_info("Using %s", *p);
			return;
		}
	}

	glext.depth_texture = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_instanced_arrays(void) {
	if(
		GL_ATLEAST(3, 3)
		&& (glext.DrawArraysInstanced = tsglDrawArraysInstanced)
		&& (glext.DrawElementsInstanced = tsglDrawElementsInstanced)
		&& (glext.VertexAttribDivisor = tsglVertexAttribDivisor)
	) {
		glext.instanced_arrays = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_instanced_arrays"))
		&& (glext.DrawArraysInstanced = tsglDrawArraysInstancedARB)
		&& (glext.DrawElementsInstanced = tsglDrawElementsInstancedARB)
		&& (glext.VertexAttribDivisor = tsglVertexAttribDivisorARB)
	) {
		log_info("Using GL_ARB_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_instanced_arrays"))
		&& (glext.DrawArraysInstanced = tsglDrawArraysInstancedEXT)
		&& (glext.DrawElementsInstanced = tsglDrawElementsInstancedEXT)
		&& (glext.VertexAttribDivisor = tsglVertexAttribDivisorEXT)
	) {
		log_info("Using GL_EXT_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ANGLE_instanced_arrays"))
		&& (glext.DrawArraysInstanced = tsglDrawArraysInstancedANGLE)
		&& (glext.DrawElementsInstanced = tsglDrawElementsInstancedANGLE)
		&& (glext.VertexAttribDivisor = tsglVertexAttribDivisorANGLE)
	) {
		log_info("Using GL_ANGLE_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_NV_instanced_arrays"))
		&& (glext.DrawArraysInstanced = tsglDrawArraysInstancedNV)
		&& (glext.DrawElementsInstanced = tsglDrawElementsInstancedNV)
		&& (glext.VertexAttribDivisor = tsglVertexAttribDivisorNV)
	) {
		log_info("Using GL_NV_instanced_arrays");
		return;
	}

	glext.instanced_arrays = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_draw_buffers(void) {
	if(
		(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0))
		&& (glext.DrawBuffers = tsglDrawBuffers)
	) {
		glext.draw_buffers = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_draw_buffers"))
		&& (glext.DrawBuffers = tsglDrawBuffersARB)
	) {
		log_info("Using GL_ARB_draw_buffers");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_draw_buffers"))
		&& (glext.DrawBuffers = tsglDrawBuffersEXT)
	) {
		log_info("Using GL_EXT_draw_buffers");
		return;
	}

	glext.draw_buffers = 0;
	log_warn("Extension not supported");
}

void glcommon_check_extensions(void) {
	memset(&glext, 0, sizeof(glext));
	glcommon_gl_version(&glext.version.major, &glext.version.minor);

	const char *glslv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	const char *glv = (const char*)glGetString(GL_VERSION);

	if(!glslv) {
		glslv = "None";
	}

	glext.version.is_es = strstartswith(glv, "OpenGL ES");

	log_info("OpenGL version: %s", glv);
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

	glcommon_ext_base_instance();
	glcommon_ext_debug_output();
	glcommon_ext_depth_texture();
	glcommon_ext_draw_buffers();
	glcommon_ext_instanced_arrays();
	glcommon_ext_pixel_buffer_object();
}

void glcommon_load_library(void) {
#ifndef LINK_TO_LIBGL
	const char *lib = env_get("TAISEI_LIBGL", "");

	if(!*lib) {
		lib = NULL;
	}

	if(SDL_GL_LoadLibrary(lib) < 0) {
		log_fatal("SDL_GL_LoadLibrary() failed: %s", SDL_GetError());
	}
#endif
}

void glcommon_unload_library(void) {
#ifndef LINK_TO_LIBGL
	SDL_GL_UnloadLibrary();
#endif
}

void glcommon_load_functions(void) {
#ifndef LINK_TO_LIBGL
#define GLDEF(glname,tsname,typename) tsname = (typename)glcommon_proc_address(#glname);
	GLDEFS
#undef GLDEF
#endif
}
