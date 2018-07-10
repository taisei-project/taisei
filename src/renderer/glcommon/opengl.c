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

typedef void (*glad_glproc_ptr)(void);

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
		&& (glext.DebugMessageCallback = glad_glDebugMessageCallback)
		&& (glext.DebugMessageControl = glad_glDebugMessageControl)
		&& (glext.ObjectLabel = glad_glObjectLabel)
	) {
		glext.debug_output = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if(glext.version.is_es) {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = glad_glDebugMessageCallbackKHR)
			&& (glext.DebugMessageControl = glad_glDebugMessageControlKHR)
			&& (glext.ObjectLabel = glad_glObjectLabelKHR)
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	} else {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = glad_glDebugMessageCallback)
			&& (glext.DebugMessageControl = glad_glDebugMessageControl)
			&& (glext.ObjectLabel = glad_glObjectLabel)
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	}

	if((glext.debug_output = glcommon_check_extension("GL_ARB_debug_output"))
		&& (glext.DebugMessageCallback = glad_glDebugMessageCallbackARB)
		&& (glext.DebugMessageControl = glad_glDebugMessageControlARB)
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
		&& (glext.DrawArraysInstancedBaseInstance = glad_glDrawArraysInstancedBaseInstance)
		&& (glext.DrawElementsInstancedBaseInstance = glad_glDrawElementsInstancedBaseInstance)
	) {
		glext.base_instance = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.base_instance = glcommon_check_extension("GL_ARB_base_instance"))
		&& (glext.DrawArraysInstancedBaseInstance = glad_glDrawArraysInstancedBaseInstance)
		&& (glext.DrawElementsInstancedBaseInstance = glad_glDrawElementsInstancedBaseInstance)
	) {
		log_info("Using GL_ARB_base_instance");
		return;
	}

	if((glext.base_instance = glcommon_check_extension("GL_EXT_base_instance"))
		&& (glext.DrawArraysInstancedBaseInstance = glad_glDrawArraysInstancedBaseInstanceEXT)
		&& (glext.DrawElementsInstancedBaseInstance = glad_glDrawElementsInstancedBaseInstanceEXT)
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
		&& (glext.DrawArraysInstanced = glad_glDrawArraysInstanced)
		&& (glext.DrawElementsInstanced = glad_glDrawElementsInstanced)
		&& (glext.VertexAttribDivisor = glad_glVertexAttribDivisor)
	) {
		glext.instanced_arrays = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_instanced_arrays"))
		&& (glext.DrawArraysInstanced = glad_glDrawArraysInstancedARB)
		&& (glext.DrawElementsInstanced = glad_glDrawElementsInstancedARB)
		&& (glext.VertexAttribDivisor = glad_glVertexAttribDivisorARB)
	) {
		log_info("Using GL_ARB_instanced_arrays (GL_ARB_draw_instanced assumed)");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_instanced_arrays"))
		&& (glext.DrawArraysInstanced = glad_glDrawArraysInstancedEXT)
		&& (glext.DrawElementsInstanced = glad_glDrawElementsInstancedEXT)
		&& (glext.VertexAttribDivisor = glad_glVertexAttribDivisorEXT)
	) {
		log_info("Using GL_EXT_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ANGLE_instanced_arrays"))
		&& (glext.DrawArraysInstanced = glad_glDrawArraysInstancedANGLE)
		&& (glext.DrawElementsInstanced = glad_glDrawElementsInstancedANGLE)
		&& (glext.VertexAttribDivisor = glad_glVertexAttribDivisorANGLE)
	) {
		log_info("Using GL_ANGLE_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_NV_instanced_arrays"))
		&& (glext.DrawArraysInstanced = glad_glDrawArraysInstancedNV)
		&& (glext.DrawElementsInstanced = glad_glDrawElementsInstancedNV)
		&& (glext.VertexAttribDivisor = glad_glVertexAttribDivisorNV)
	) {
		log_info("Using GL_NV_instanced_arrays (GL_NV_draw_instanced assumed)");
		return;
	}

	glext.instanced_arrays = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_draw_buffers(void) {
	if(
		(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0))
		&& (glext.DrawBuffers = glad_glDrawBuffers)
	) {
		glext.draw_buffers = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_draw_buffers"))
		&& (glext.DrawBuffers = glad_glDrawBuffersARB)
	) {
		log_info("Using GL_ARB_draw_buffers");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_draw_buffers"))
		&& (glext.DrawBuffers = glad_glDrawBuffersEXT)
	) {
		log_info("Using GL_EXT_draw_buffers");
		return;
	}

	glext.draw_buffers = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_texture_filter_anisotropic(void) {
	if(GL_ATLEAST(4, 6)) {
		glext.texture_filter_anisotropic = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.texture_filter_anisotropic = glcommon_check_extension("GL_ARB_texture_filter_anisotropic"))) {
		log_info("Using ARB_texture_filter_anisotropic");
		return;
	}

	if((glext.texture_filter_anisotropic = glcommon_check_extension("GL_EXT_texture_filter_anisotropic"))) {
		log_info("Using EXT_texture_filter_anisotropic");
		return;
	}

	glext.texture_filter_anisotropic = 0;
	log_warn("Extension not supported");
}

void glcommon_check_extensions(void) {
	memset(&glext, 0, sizeof(glext));
	glext.version.major = GLVersion.major;
	glext.version.minor = GLVersion.minor;

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
	glcommon_ext_texture_filter_anisotropic();
}

void glcommon_load_library(void) {
	const char *lib = env_get("TAISEI_LIBGL", "");

	if(!*lib) {
		lib = NULL;
	}

	if(SDL_GL_LoadLibrary(lib) < 0) {
		log_fatal("SDL_GL_LoadLibrary() failed: %s", SDL_GetError());
	}
}

void glcommon_unload_library(void) {
	SDL_GL_UnloadLibrary();
}

void glcommon_load_functions(void) {
	int profile;

	if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) < 0) {
		log_fatal("SDL_GL_GetAttribute() failed: %s", SDL_GetError());
	}

	if(profile == SDL_GL_CONTEXT_PROFILE_ES) {
		if(!gladLoadGLES2Loader(SDL_GL_GetProcAddress)) {
			log_fatal("Failed to load OpenGL ES functions");
		}
	} else {
		if(!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
			log_fatal("Failed to load OpenGL functions");
		}
	}
}
