/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "rwops/rwops_autobuf.h"
#include "opengl.h"
#include "debug.h"
#include "shaders.h"

// undef extension abstraction macros
#undef glDebugMessageControl
#undef glDebugMessageCallback
#undef glObjectLabel
#undef glDrawArraysInstanced
#undef glDrawElementsInstanced
#undef glVertexAttribDivisor
#undef glDrawArraysInstancedBaseInstance
#undef glDrawElementsInstancedBaseInstance
#undef glDrawBuffers
#undef glClearTexImage
// #undef glClearTexSubImage
#undef glBindVertexArray
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glIsVertexArray
#undef glGetFloati_v
#undef glViewportIndexedfv

struct glext_s glext;

typedef void (*glad_glproc_ptr)(void);

// WEBGL_debug_renderer_info
const GLenum GL_UNMASKED_VENDOR_WEBGL = 0x9245;
const GLenum GL_UNMASKED_RENDERER_WEBGL = 0x9246;

static const char *const ext_vendor_table[] = {
	#define TSGL_EXT_VENDOR(v) [_TSGL_EXTVNUM_##v] = #v,
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR

	NULL,
};

attr_nonnull_all
static ext_flag_t glcommon_ext_flag(const char *ext) {
	const char *ext_orig = ext;
	ext = strchr(ext, '_');

	if(ext == NULL) {
		log_fatal("Bad extension string: %s", ext_orig);
	}

	const char *sep = strchr(++ext, '_');

	if(sep == NULL) {
		log_fatal("Bad extension string: %s", ext_orig);
	}

	char vendor[sep - ext + 1];
	strlcpy(vendor, ext, sizeof(vendor));

	for(const char *const *p = ext_vendor_table; *p; ++p) {
		if(!strcmp(*p, vendor)) {
			return 1 << (p - ext_vendor_table);
		}
	}

	log_fatal("Unknown vendor '%s' in extension string %s", vendor, ext_orig);
}

ext_flag_t glcommon_check_extension(const char *ext) {
	assert(*ext != 0);
	assert(strchr(ext, ' ') == NULL);

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

	// SDL_GL_ExtensionSupported is stupid and requires dlopen() with emscripten.
	// Let's reinvent it!

	// SDL does this
	if(env_get_int(ext, 1) == 0) {
		return 0;
	}

#ifndef STATIC_GLES3
	if(GL_ATLEAST(3, 0) || GLES_ATLEAST(3, 0))
#endif
	{
		GLint num_exts = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
		for(GLint i = 0; i < num_exts; ++i) {
			const char *e = (const char*)glGetStringi(GL_EXTENSIONS, i);
			if(!strcmp(ext, e)) {
				return flag;
			}
		}

		return 0;
	}

#ifndef STATIC_GLES3
	// The legacy way
	const char *extensions = (const char*)glGetString(GL_EXTENSIONS);

	if(!extensions) {
		return 0;
	}

	const char *start = extensions;
	size_t ext_len = strlen(ext);

	for(;;) {
		const char *where = strstr(start, ext);
		if(!where) {
			return 0;
		}

		const char *term = where + ext_len;

		if(
			(where == extensions || where[-1] == ' ') &&
			(*term == ' ' || *term == '\0')
		) {
			return flag;
		}
	}
#endif
}

ext_flag_t glcommon_require_extension(const char *ext) {
	ext_flag_t val = glcommon_check_extension(ext);

	if(!val) {
		if(env_get("TAISEI_GL_REQUIRE_EXTENSION_FATAL", 0)) {
			log_fatal("Required extension %s is not available", ext);
		}

		log_error("Required extension %s is not available, expect crashes or rendering errors", ext);
	}

	return val;
}

static void glcommon_ext_debug_output(void) {
#ifndef STATIC_GLES3
	if(
		GL_ATLEAST(4, 3)
		&& (glext.DebugMessageCallback = GL_FUNC(DebugMessageCallback))
		&& (glext.DebugMessageControl = GL_FUNC(DebugMessageControl))
		&& (glext.ObjectLabel = GL_FUNC(ObjectLabel))
	) {
		glext.debug_output = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if(glext.version.is_es) {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = GL_FUNC(DebugMessageCallbackKHR))
			&& (glext.DebugMessageControl = GL_FUNC(DebugMessageControlKHR))
			&& (glext.ObjectLabel = GL_FUNC(ObjectLabelKHR))
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	} else {
		if((glext.debug_output = glcommon_check_extension("GL_KHR_debug"))
			&& (glext.DebugMessageCallback = GL_FUNC(DebugMessageCallback))
			&& (glext.DebugMessageControl = GL_FUNC(DebugMessageControl))
			&& (glext.ObjectLabel = GL_FUNC(ObjectLabel))
		) {
			log_info("Using GL_KHR_debug");
			return;
		}
	}

	if((glext.debug_output = glcommon_check_extension("GL_ARB_debug_output"))
		&& (glext.DebugMessageCallback = GL_FUNC(DebugMessageCallbackARB))
		&& (glext.DebugMessageControl = GL_FUNC(DebugMessageControlARB))
	) {
		log_info("Using GL_ARB_debug_output");
		return;
	}
#endif

	glext.debug_output = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_pixel_buffer_object(void) {
#ifdef STATIC_GLES3
	glext.pixel_buffer_object = TSGL_EXTFLAG_NATIVE;
	log_info("Using core functionality");
	return;
#else
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
#endif
}

static void glcommon_ext_depth_texture(void) {
#ifdef STATIC_GLES3
	glext.depth_texture = TSGL_EXTFLAG_NATIVE;
	log_info("Using core functionality");
	return;
#else
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
#endif
}

static void glcommon_ext_instanced_arrays(void) {
#ifdef STATIC_GLES3
	glext.instanced_arrays = TSGL_EXTFLAG_NATIVE;
	log_info("Using core functionality");
	return;
#else
	if(
		(GL_ATLEAST(3, 3) || GLES_ATLEAST(3, 0))
		&& (glext.DrawArraysInstanced = GL_FUNC(DrawArraysInstanced))
		&& (glext.DrawElementsInstanced = GL_FUNC(DrawElementsInstanced))
		&& (glext.VertexAttribDivisor = GL_FUNC(VertexAttribDivisor))
	) {
		glext.instanced_arrays = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_instanced_arrays"))
		&& (glext.DrawArraysInstanced = GL_FUNC(DrawArraysInstancedARB))
		&& (glext.DrawElementsInstanced = GL_FUNC(DrawElementsInstancedARB))
		&& (glext.VertexAttribDivisor = GL_FUNC(VertexAttribDivisorARB))
	) {
		log_info("Using GL_ARB_instanced_arrays (GL_ARB_draw_instanced assumed)");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_instanced_arrays"))
		&& (glext.DrawArraysInstanced = GL_FUNC(DrawArraysInstancedEXT))
		&& (glext.DrawElementsInstanced = GL_FUNC(DrawElementsInstancedEXT))
		&& (glext.VertexAttribDivisor = GL_FUNC(VertexAttribDivisorEXT))
	) {
		log_info("Using GL_EXT_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ANGLE_instanced_arrays"))
		&& (glext.DrawArraysInstanced = GL_FUNC(DrawArraysInstancedANGLE))
		&& (glext.DrawElementsInstanced = GL_FUNC(DrawElementsInstancedANGLE))
		&& (glext.VertexAttribDivisor = GL_FUNC(VertexAttribDivisorANGLE))
	) {
		log_info("Using GL_ANGLE_instanced_arrays");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_NV_instanced_arrays"))
		&& (glext.DrawArraysInstanced = GL_FUNC(DrawArraysInstancedNV))
		&& (glext.DrawElementsInstanced = GL_FUNC(DrawElementsInstancedNV))
		&& (glext.VertexAttribDivisor = GL_FUNC(VertexAttribDivisorNV))
	) {
		log_info("Using GL_NV_instanced_arrays (GL_NV_draw_instanced assumed)");
		return;
	}

	glext.instanced_arrays = 0;
	log_warn("Extension not supported");
#endif
}

static void glcommon_ext_draw_buffers(void) {
#ifdef STATIC_GLES3
	glext.draw_buffers = TSGL_EXTFLAG_NATIVE;
	log_info("Using core functionality");
	return;
#else
	if(
		(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0))
		&& (glext.DrawBuffers = GL_FUNC(DrawBuffers))
	) {
		glext.draw_buffers = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_ARB_draw_buffers"))
		&& (glext.DrawBuffers = GL_FUNC(DrawBuffersARB))
	) {
		log_info("Using GL_ARB_draw_buffers");
		return;
	}

	if((glext.instanced_arrays = glcommon_check_extension("GL_EXT_draw_buffers"))
		&& (glext.DrawBuffers = GL_FUNC(DrawBuffersEXT))
	) {
		log_info("Using GL_EXT_draw_buffers");
		return;
	}

	glext.draw_buffers = 0;
	log_warn("Extension not supported");
#endif
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

static void glcommon_ext_clear_texture(void) {
#ifdef STATIC_GLES3
	if((glext.clear_texture = glcommon_check_extension("GL_EXT_clear_texture"))) {
		log_info("Using GL_EXT_clear_texture");
		return;
	}
#else
	if(
		GL_ATLEAST(4, 4)
		&& (glext.ClearTexImage = GL_FUNC(ClearTexImage))
	) {
		glext.clear_texture = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.clear_texture = glcommon_check_extension("GL_ARB_clear_texture"))
		&& (glext.ClearTexImage = GL_FUNC(ClearTexImage))
	) {
		log_info("Using GL_ARB_clear_texture");
		return;
	}

	if((glext.clear_texture = glcommon_check_extension("GL_EXT_clear_texture"))
		&& (glext.ClearTexImage = GL_FUNC(ClearTexImageEXT))
	) {
		log_info("Using GL_EXT_clear_texture");
		return;
	}
#endif

	glext.clear_texture = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_texture_norm16(void) {
	if(!glext.version.is_es) {
		glext.texture_norm16 = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	/*
	 * Quote from the WEBGL_EXT_texture_norm16 spec
	 * (https://www.khronos.org/registry/webgl/extensions/EXT_texture_norm16/):
	 *
	 * This extension provides a set of new 16-bit signed normalized and unsigned normalized fixed
	 * point texture, renderbuffer and texture buffer formats. The 16-bit normalized fixed point
	 * types R16_EXT, RG16_EXT and RGBA16_EXT become available as color-renderable formats.
	 * Renderbuffers can be created in these formats. These and textures created with type =
	 * UNSIGNED_SHORT, which will have one of these internal formats, can be attached to
	 * framebuffer object color attachments for rendering.
	 *
	 * The wording suggests that both renderbuffers and textures can be attached to a framebuffer
	 * object. However, Chromium/ANGLE seems to violate the spec here and does not permit textures.
	 *
	 * Exclude WebGL to be safe.
	 */
	if(
		!glext.version.is_webgl &&
		(glext.texture_norm16 = glcommon_check_extension("GL_EXT_texture_norm16"))
	) {
		log_info("Using GL_EXT_texture_norm16");
		return;
	}

	glext.texture_norm16 = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_texture_rg(void) {
	if(!glext.version.is_es) {
		glext.texture_rg = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.texture_rg = glcommon_check_extension("GL_EXT_texture_rg"))) {
		log_info("Using GL_EXT_texture_rg");
		return;
	}

	glext.texture_rg = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_texture_float_linear(void) {
	if(!glext.version.is_es) {
		glext.texture_float_linear = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.texture_float_linear = glcommon_check_extension("GL_OES_texture_float_linear"))) {
		log_info("Using GL_OES_texture_float_linear");
		return;
	}

	glext.texture_float_linear = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_texture_half_float_linear(void) {
	if(!glext.version.is_es) {
		glext.texture_half_float_linear = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.texture_half_float_linear = glcommon_check_extension("GL_OES_texture_half_float_linear"))) {
		log_info("Using GL_OES_texture_half_float_linear");
		return;
	}

	glext.texture_half_float_linear = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_color_buffer_float(void) {
	if(!glext.version.is_es) {
		glext.color_buffer_float = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.color_buffer_float = glcommon_check_extension("GL_EXT_color_buffer_float"))) {
		log_info("Using GL_EXT_color_buffer_float");
		return;
	}

	glext.color_buffer_float = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_float_blend(void) {
	if(!glext.version.is_es) {
		glext.float_blend = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.float_blend = glcommon_check_extension("GL_EXT_float_blend"))) {
		log_info("Using GL_EXT_float_blend");
		return;
	}

	glext.float_blend = 0;
	log_warn("Extension not supported");
}

static void glcommon_ext_vertex_array_object(void) {
#ifdef STATIC_GLES3
	glext.vertex_array_object = TSGL_EXTFLAG_NATIVE;
	log_info("Using core functionality");
	return;
#else
	if((GL_ATLEAST(3, 0) || GLES_ATLEAST(3, 0))
		&& (glext.BindVertexArray = GL_FUNC(BindVertexArray))
		&& (glext.DeleteVertexArrays = GL_FUNC(DeleteVertexArrays))
		&& (glext.GenVertexArrays = GL_FUNC(GenVertexArrays))
		&& (glext.IsVertexArray = GL_FUNC(IsVertexArray))
	) {
		glext.vertex_array_object = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.vertex_array_object = glcommon_check_extension("GL_ARB_vertex_array_object"))
		&& (glext.BindVertexArray = GL_FUNC(BindVertexArray))
		&& (glext.DeleteVertexArrays = GL_FUNC(DeleteVertexArrays))
		&& (glext.GenVertexArrays = GL_FUNC(GenVertexArrays))
		&& (glext.IsVertexArray = GL_FUNC(IsVertexArray))
	) {
		log_info("Using GL_ARB_vertex_array_object");
		return;
	}

	if((glext.vertex_array_object = glcommon_check_extension("GL_OES_vertex_array_object"))
		&& (glext.BindVertexArray = GL_FUNC(BindVertexArrayOES))
		&& (glext.DeleteVertexArrays = GL_FUNC(DeleteVertexArraysOES))
		&& (glext.GenVertexArrays = GL_FUNC(GenVertexArraysOES))
		&& (glext.IsVertexArray = GL_FUNC(IsVertexArrayOES))
	) {
		log_info("Using GL_OES_vertex_array_object");
		return;
	}

	if((glext.vertex_array_object = glcommon_check_extension("GL_APPLE_vertex_array_object"))
		&& (glext.BindVertexArray = GL_FUNC(BindVertexArrayAPPLE))
		&& (glext.DeleteVertexArrays = GL_FUNC(DeleteVertexArraysAPPLE))
		&& (glext.GenVertexArrays = GL_FUNC(GenVertexArraysAPPLE))
		&& (glext.IsVertexArray = GL_FUNC(IsVertexArrayAPPLE))
	) {
		log_info("Using GL_APPLE_vertex_array_object");
		return;
	}

	glext.vertex_array_object = 0;
	log_warn("Extension not supported");
#endif
}

static void glcommon_ext_viewport_array(void) {
#ifndef STATIC_GLES3
	if((GL_ATLEAST(4, 1))
		&& (glext.GetFloati_v = GL_FUNC(GetFloati_v))
		&& (glext.ViewportIndexedfv = GL_FUNC(ViewportIndexedfv))
	) {
		glext.viewport_array = TSGL_EXTFLAG_NATIVE;
		log_info("Using core functionality");
		return;
	}

	if((glext.viewport_array = glcommon_check_extension("GL_ARB_viewport_array"))
		&& (glext.GetFloati_v = GL_FUNC(GetFloati_v))
		&& (glext.ViewportIndexedfv = GL_FUNC(ViewportIndexedfv))
	) {
		log_info("Using GL_ARB_viewport_array");
		return;
	}

	if((glext.viewport_array = glcommon_check_extension("GL_OES_viewport_array"))
		&& (glext.GetFloati_v = GL_FUNC(GetFloati_vOES))
		&& (glext.ViewportIndexedfv = GL_FUNC(ViewportIndexedfvOES))
	) {
		log_info("Using GL_OES_viewport_array");
		return;
	}
#endif

	glext.viewport_array = 0;
	log_warn("Extension not supported");
}

#ifndef STATIC_GLES3
static APIENTRY GLvoid shim_glClearDepth(GLdouble depthval) {
	glClearDepthf(depthval);
}

static APIENTRY GLvoid shim_glClearDepthf(GLfloat depthval) {
	glClearDepth(depthval);
}
#endif

static const char *get_unmasked_property(GLenum prop, bool fallback) {
	const char *val = NULL;

	if(glext.version.is_webgl) {
		if(glcommon_check_extension("GL_WEBGL_debug_renderer_info")) {
			GLenum prop_unmasked;

			switch(prop) {
				case GL_VENDOR:   prop_unmasked = GL_UNMASKED_VENDOR_WEBGL;   break;
				case GL_RENDERER: prop_unmasked = GL_UNMASKED_RENDERER_WEBGL; break;
				default: UNREACHABLE;
			}

			val = (const char*)glGetString(prop_unmasked);

			if(!*val) {
				val = NULL;
			}
		}
	}

	if(val == NULL && fallback) {
		val = (const char*)glGetString(prop);
	}

	return val;
}

static void detect_broken_intel_driver(void) {
#ifdef TAISEI_BUILDCONF_WINDOWS_ANGLE_INTEL
	extern DECLSPEC int SDLCALL SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid);

	bool is_broken_intel_driver = (
		!glext.version.is_es &&
		strstartswith((const char*)glGetString(GL_VENDOR), "Intel") &&
		strstr((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION), " - Build ")
	);

	if(!is_broken_intel_driver) {
		return;
	}

	int button;
	SDL_MessageBoxData mbdata = { 0 };

	mbdata.flags = SDL_MESSAGEBOX_WARNING;
	mbdata.title = "Taisei Project";

	char *msg = strfmt(
		"Looks like you have a broken OpenGL driver.\n"
		"Taisei will probably not work correctly, if at all.\n\n"
		"Starting the game in ANGLE mode should fix the problem, but may introduce slowdown.\n\n"
		"Restart in ANGLE mode now? (If unsure, press YES)"
	);

	mbdata.message = msg;
	mbdata.numbuttons = 3;
	mbdata.buttons = (SDL_MessageBoxButtonData[]) {
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Abort" },
		{ 0, 1, "No" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes (safe)" },
	};

	int mbresult = SDL_ShowMessageBox(&mbdata, &button);
	free(msg);

	if(mbresult < 0) {
		log_sdl_error(LOG_ERROR, "SDL_ShowMessageBox");
	} else if(button == 1) {
		return;
	} else if(button == 2) {
		exit(1);
	}

	const WCHAR *cmdline = GetCommandLine();
	const WCHAR renderer_args[] = L" --renderer gles30";
	WCHAR new_cmdline[wcslen(cmdline) + wcslen(renderer_args) + 1];
	memcpy(new_cmdline, cmdline, sizeof(WCHAR) * wcslen(cmdline));
	memcpy(new_cmdline + wcslen(cmdline), renderer_args, sizeof(renderer_args));

	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };

	CreateProcessW(
		NULL,
		new_cmdline,
		NULL,
		NULL,
		false,
		0,
		NULL,
		NULL,
		&si,
		&pi
	);

	exit(0);

#endif
}

static bool glcommon_check_workaround(const char *name, const char *envvar, const char *(*detect)(void)) {
	int env_setting = env_get_int(envvar, -1);
	glext.issues.avoid_sampler_uniform_updates = false;

	if(env_setting > 0) {
		log_warn("Enabled workaround `%s` (forced by environment)", name);
		return true;
	}

	if(env_setting == 0) {
		log_warn("Disabled workaround `%s` (forced by environment)", name);
		return false;
	}

	const char *reason = detect();
	if(reason != NULL) {
		log_warn("Enabled workaround `%s` (%s)", name, reason);
		return true;
	}

	log_info("Workaround `%s` not needed", name);
	return false;
}

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
EM_JS(bool, webgl_is_mac, (void), {
	try {
		return !!navigator.platform.match(/mac/i);
	} catch(e) {
		// whatever...
		return false;
	}
})
#endif

static const char *detect_slow_sampler_update(void) {
#if defined(__MACOSX__) || defined(__EMSCRIPTEN__)
	const char *gl_vendor = get_unmasked_property(GL_VENDOR, true);
	const char *gl_renderer = get_unmasked_property(GL_RENDERER, true);

	if(
#if defined(__EMSCRIPTEN__)
		webgl_is_mac() &&
#endif
		strstr(gl_renderer, "Radeon") && (                                  // This looks like an AMD Radeon card...
			(strstr(gl_vendor, "ATI") || strstr(gl_vendor, "AMD")) ||       // ...and AMD's official driver...
			(strstr(gl_vendor, "Google") && strstr(gl_renderer, "OpenGL"))  // ...or ANGLE, backed by OpenGL.
		)
	) {
		return "buggy AMD driver on macOS; see https://github.com/taisei-project/taisei/issues/182";
	}
#endif
	return NULL;
}

static void glcommon_check_issues(void) {
	glext.issues.avoid_sampler_uniform_updates = glcommon_check_workaround(
		"avoid sampler uniform updates",
		"TAISEI_GL_WORKAROUND_AVOID_SAMPLER_UNIFORM_UPDATES",
		detect_slow_sampler_update
	);
}

void glcommon_check_capabilities(void) {
	memset(&glext, 0, sizeof(glext));

	const char *glslv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	const char *glv = (const char*)glGetString(GL_VERSION);

#ifdef STATIC_GLES3
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	glext.version.major = major;
	glext.version.minor = minor;
#else
	glext.version.major = GLVersion.major;
	glext.version.minor = GLVersion.minor;
#endif

	if(!glslv) {
		glslv = "None";
	}

	glext.version.is_es = strstartswith(glv, "OpenGL ES");

#ifdef STATIC_GLES3
	if(!GLES_ATLEAST(3, 0)) {
		log_fatal("Compiled with STATIC_GLES3, but got a non-GLES3 context. Can't work with this");
	}
#endif

	if(glext.version.is_es) {
		glext.version.is_ANGLE = strstr(glv, "(ANGLE ");
		glext.version.is_webgl = strstr(glv, "(WebGL ");
	}

	log_info("OpenGL version: %s", glv);
	log_info("OpenGL vendor: %s", (const char*)glGetString(GL_VENDOR));
	log_info("OpenGL renderer: %s", (const char*)glGetString(GL_RENDERER));

	if(glext.version.is_webgl) {
		const char *unmasked_vendor = get_unmasked_property(GL_VENDOR, false);
		const char *unmasked_renderer = get_unmasked_property(GL_RENDERER, false);
		log_info("OpenGL unmasked vendor: %s",   unmasked_vendor   ? unmasked_vendor   : "Unknown");
		log_info("OpenGL unmasked renderer: %s", unmasked_renderer ? unmasked_renderer : "Unknown");
	}

	log_info("GLSL version: %s", glslv);

	detect_broken_intel_driver();

	// XXX: this is the legacy way, maybe we shouldn't try this first
	const char *exts = (const char*)glGetString(GL_EXTENSIONS);

	if(exts) {
		log_info("Supported extensions: %s", exts);
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
		log_info("%s", (char*)buf);
		SDL_RWclose(writer);
	}

	glcommon_ext_clear_texture();
	glcommon_ext_color_buffer_float();
	glcommon_ext_debug_output();
	glcommon_ext_depth_texture();
	glcommon_ext_draw_buffers();
	glcommon_ext_float_blend();
	glcommon_ext_instanced_arrays();
	glcommon_ext_pixel_buffer_object();
	glcommon_ext_texture_filter_anisotropic();
	glcommon_ext_texture_float_linear();
	glcommon_ext_texture_half_float_linear();
	glcommon_ext_texture_norm16();
	glcommon_ext_texture_rg();
	glcommon_ext_vertex_array_object();
	glcommon_ext_viewport_array();

	// GLES has only glClearDepthf
	// Core has only glClearDepth until GL 4.1
#ifndef STATIC_GLES3
	assert(glClearDepth || glClearDepthf);

	if(!glClearDepth) {
		glClearDepth = shim_glClearDepth;
	}

	if(!glClearDepthf) {
		glClearDepthf = shim_glClearDepthf;
	}
#endif

	glcommon_build_shader_lang_table();
	glcommon_check_issues();
}

void glcommon_load_library(void) {
#ifndef STATIC_GLES3
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
#ifndef STATIC_GLES3
	SDL_GL_UnloadLibrary();
#endif
	glcommon_free_shader_lang_table();
}

attr_unused
static inline void (*load_func(const char *name))(void) {
	union {
		void *vp;
		void (*fp)(void);
	} c_sucks;
	c_sucks.vp = SDL_GL_GetProcAddress(name);
	return c_sucks.fp;
}

void glcommon_load_functions(void) {
#ifndef STATIC_GLES3
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
#endif
}

void glcommon_setup_attributes(SDL_GLprofile profile, uint major, uint minor, SDL_GLcontextFlag ctxflags) {
	if(glcommon_debug_requested()) {
		ctxflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);

#ifndef __EMSCRIPTEN__
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, ctxflags);
#endif
}
