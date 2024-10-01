/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "opengl.h"

#include "debug.h"
#include "rwops/rwops_autobuf.h"
#include "shaders.h"
#include "texture.h"
#include "util/env.h"
#include "util/io.h"

struct glext_s glext = { 0 };

typedef void (*glad_glproc_ptr)(void);

#ifndef STATIC_GLES3
//
// shims
//

APIENTRY
static void shim_glClearDepth(GLdouble depthval) {
	glClearDepthf(depthval);
}

APIENTRY
static void shim_glClearDepthf(GLfloat depthval) {
	glClearDepth(depthval);
}
#endif

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

		start = term;
	}
#endif
}

#define EXT_FLAG(flagname) \
	ext_flag_t *pExtField = &glext.flagname

#define CHECK_CORE(cond) do { \
	if((cond)) { \
		*pExtField = TSGL_EXTFLAG_NATIVE; \
		log_info("Using core functionality"); \
		return; \
	} \
} while(0)

#define CHECK_EXT(extname) do { \
	if((*pExtField = glcommon_check_extension(#extname))) { \
		log_info("Using " #extname); \
		return; \
	} \
} while(0)

#define EXT_MISSING() do { \
	*pExtField = 0; \
	log_warn("Extension not supported"); \
} while(0)

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
	EXT_FLAG(debug_output);

#ifndef STATIC_GLES3
	if(HAVE_GL_FUNC(glDebugMessageCallback) && HAVE_GL_FUNC(glDebugMessageControl)) {
		if(HAVE_GL_FUNC(glObjectLabel)) {
			CHECK_CORE(GL_ATLEAST(4, 3));
			CHECK_EXT(GL_KHR_debug);
		}

		CHECK_EXT(GL_ARB_debug_output);
	}
#endif

	EXT_MISSING();
}

static void glcommon_ext_pixel_buffer_object(void) {
	EXT_FLAG(pixel_buffer_object);

#ifdef STATIC_GLES3
	CHECK_CORE(1);
#else
	// TODO: verify that these requirements are correct
	CHECK_CORE(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_ARB_pixel_buffer_object);
	CHECK_EXT(GL_EXT_pixel_buffer_object);
	CHECK_EXT(GL_NV_pixel_buffer_object);
	EXT_MISSING();
#endif
}

static void glcommon_ext_seamless_cubemap(void) {
	EXT_FLAG(seamless_cubemap);

#ifndef STATIC_GLES3
	// NOTE: GLES3 seems to provide seamless sampling by default,
	// but it must be glEnabled in plain OpenGL, which is what this extension is about.

	CHECK_CORE(GL_ATLEAST(3, 2));
	CHECK_EXT(GL_ARB_seamless_cube_map);
#endif

	EXT_MISSING();
}

static void glcommon_ext_depth_texture(void) {
	EXT_FLAG(depth_texture);

#ifdef STATIC_GLES3
	CHECK_CORE(1);
#else
	// TODO: detect this for core OpenGL properly
	CHECK_CORE(!glext.version.is_es || GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_OES_depth_texture);
	CHECK_EXT(GL_ANGLE_depth_texture);
	CHECK_EXT(GL_SGIX_depth_texture);
	EXT_MISSING();
#endif
}

static void glcommon_ext_instanced_arrays(void) {
	EXT_FLAG(instanced_arrays);

#ifdef STATIC_GLES3
	CHECK_CORE(1);
#else
	if(
		HAVE_GL_FUNC(glDrawArraysInstanced) &&
		HAVE_GL_FUNC(glDrawElementsInstanced) &&
		HAVE_GL_FUNC(glVertexAttribDivisor)
	) {
		CHECK_CORE(GL_ATLEAST(3, 3) || GLES_ATLEAST(3, 0));
		CHECK_EXT(GL_ANGLE_instanced_arrays);
		CHECK_EXT(GL_ARB_instanced_arrays);
		CHECK_EXT(GL_EXT_instanced_arrays);
		CHECK_EXT(GL_NV_instanced_arrays);
	}

	EXT_MISSING();
#endif
}

static void glcommon_ext_internalformat_query2(void) {
	EXT_FLAG(internalformat_query2);

#ifndef STATIC_GLES3
	CHECK_CORE(GL_ATLEAST(4, 3));
	CHECK_EXT(GL_ARB_internalformat_query2);
#endif

	EXT_MISSING();
}

static void glcommon_ext_draw_buffers(void) {
	EXT_FLAG(draw_buffers);

#ifdef STATIC_GLES3
	CHECK_CORE(1);
#else
	if(HAVE_GL_FUNC(glDrawBuffers)) {
		CHECK_CORE(GL_ATLEAST(2, 0) || GLES_ATLEAST(3, 0));
		CHECK_EXT(GL_ARB_draw_buffers);
		CHECK_EXT(GL_EXT_draw_buffers);
	}

	EXT_MISSING();
#endif
}

static void glcommon_ext_texture_filter_anisotropic(void) {
	EXT_FLAG(texture_filter_anisotropic);

	CHECK_CORE(GL_ATLEAST(4, 6));
	CHECK_EXT(GL_ARB_texture_filter_anisotropic);
	CHECK_EXT(GL_EXT_texture_filter_anisotropic);

	EXT_MISSING();
}

static void glcommon_ext_clear_texture(void) {
	EXT_FLAG(clear_texture);

	if(HAVE_GL_FUNC(glClearTexImage)) {
		CHECK_CORE(GL_ATLEAST(4, 4));
		CHECK_EXT(GL_ARB_clear_texture);
		CHECK_EXT(GL_EXT_clear_texture);
	}

	EXT_MISSING();
}

static void glcommon_ext_texture_norm16(void) {
	EXT_FLAG(texture_norm16);

	if(glext.issues.disable_norm16) {
		EXT_MISSING();
		return;
	}

	CHECK_CORE(!glext.version.is_es);

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
	if(!glext.version.is_webgl) {
		CHECK_EXT(GL_EXT_texture_norm16);
	}

	EXT_MISSING();
}

static void glcommon_ext_texture_rg(void) {
	EXT_FLAG(texture_rg);

	CHECK_CORE(!glext.version.is_es);
	CHECK_EXT(GL_EXT_texture_rg);

	EXT_MISSING();
}

static void glcommon_ext_texture_swizzle(void) {
	EXT_FLAG(texture_swizzle);

	CHECK_CORE(GL_ATLEAST(3, 3) || (GLES_ATLEAST(3, 0) && !glext.version.is_webgl));
	CHECK_EXT(GL_ARB_texture_swizzle);
	CHECK_EXT(GL_EXT_texture_swizzle);

	EXT_MISSING();
}

static void glcommon_ext_texture_float(void) {
	EXT_FLAG(texture_float);

	CHECK_CORE(!glext.version.is_es || GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_OES_texture_float);

	EXT_MISSING();
}

static void glcommon_ext_texture_half_float(void) {
	EXT_FLAG(texture_half_float);

	CHECK_CORE(!glext.version.is_es || GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_OES_texture_half_float);

	EXT_MISSING();
}

static void glcommon_ext_texture_float_linear(void) {
	EXT_FLAG(texture_float_linear);

	CHECK_CORE(!glext.version.is_es);
	CHECK_EXT(GL_OES_texture_float_linear);

	EXT_MISSING();
}

static void glcommon_ext_texture_half_float_linear(void) {
	EXT_FLAG(texture_half_float_linear);

	CHECK_CORE(!glext.version.is_es);
	CHECK_EXT(GL_OES_texture_half_float_linear);

	EXT_MISSING();
}

static void glcommon_ext_color_buffer_float(void) {
	EXT_FLAG(color_buffer_float);

	CHECK_CORE(!glext.version.is_es);
	CHECK_EXT(GL_EXT_color_buffer_float);

	EXT_MISSING();
}

static void glcommon_ext_float_blend(void) {
	EXT_FLAG(float_blend);

	CHECK_CORE(!glext.version.is_es);
	CHECK_EXT(GL_EXT_float_blend);

	EXT_MISSING();
}

static void glcommon_ext_vertex_array_object(void) {
	EXT_FLAG(vertex_array_object);

#ifdef STATIC_GLES3
	CHECK_CORE(1);
#else
	if(
		HAVE_GL_FUNC(glBindVertexArray) &&
		HAVE_GL_FUNC(glDeleteVertexArrays) &&
		HAVE_GL_FUNC(glGenVertexArrays) &&
		HAVE_GL_FUNC(glIsVertexArray)
	) {
		CHECK_CORE(GL_ATLEAST(3, 0) || GLES_ATLEAST(3, 0));
		CHECK_EXT(GL_ARB_vertex_array_object);
		CHECK_EXT(GL_OES_vertex_array_object);
	}

	EXT_MISSING();
#endif
}

static void glcommon_ext_viewport_array(void) {
	EXT_FLAG(viewport_array);

#ifndef STATIC_GLES3
	if(
		HAVE_GL_FUNC(glGetFloati_v) &&
		HAVE_GL_FUNC(glViewportIndexedfv)
	) {
		CHECK_CORE(GL_ATLEAST(4, 1));
		CHECK_EXT(GL_ARB_viewport_array);
		CHECK_EXT(GL_OES_viewport_array);
		CHECK_EXT(GL_NV_viewport_array2);
	}
#endif

	EXT_MISSING();
}

static void glcommon_ext_texture_format_r8_srgb(void) {
	EXT_FLAG(tex_format.r8_srgb);

	CHECK_EXT(GL_EXT_texture_sRGB_R8);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_rg8_srgb(void) {
	EXT_FLAG(tex_format.rg8_srgb);

	CHECK_EXT(GL_EXT_texture_sRGB_RG8);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_rgb8_rgba8_srgb(void) {
	EXT_FLAG(tex_format.rgb8_rgba8_srgb);

	CHECK_CORE(GL_ATLEAST(3, 0) || GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_EXT_texture_sRGB);
	CHECK_EXT(GL_EXT_sRGB);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_s3tc_dx1(void) {
	EXT_FLAG(tex_format.s3tc_dx1);

	CHECK_EXT(GL_EXT_texture_compression_s3tc);
	CHECK_EXT(GL_NV_texture_compression_s3tc);
	CHECK_EXT(GL_EXT_texture_compression_dxt1);
	CHECK_EXT(GL_ANGLE_texture_compression_dxt1);
	CHECK_EXT(GL_WEBGL_compressed_texture_s3tc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_s3tc_dx5(void) {
	EXT_FLAG(tex_format.s3tc_dx5);

	CHECK_EXT(GL_EXT_texture_compression_s3tc);
	CHECK_EXT(GL_NV_texture_compression_s3tc);
	CHECK_EXT(GL_ANGLE_texture_compression_dxt5);
	CHECK_EXT(GL_WEBGL_compressed_texture_s3tc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_s3tc_srgb(void) {
	EXT_FLAG(tex_format.s3tc_srgb);

	if(glext.tex_format.s3tc_dx1 || glext.tex_format.s3tc_dx5) {
		CHECK_EXT(GL_EXT_texture_sRGB);
		CHECK_EXT(GL_EXT_texture_compression_s3tc_srgb);
		CHECK_EXT(GL_NV_sRGB_formats);
		CHECK_EXT(GL_WEBGL_compressed_texture_s3tc_srgb);
	}

	EXT_MISSING();
}

static void glcommon_ext_texture_format_rgtc(void) {
	EXT_FLAG(tex_format.rgtc);

	CHECK_CORE(GL_ATLEAST(3, 0));
	CHECK_EXT(GL_EXT_texture_compression_rgtc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_etc1(void) {
	EXT_FLAG(tex_format.etc1);

	CHECK_EXT(GL_OES_compressed_ETC1_RGB8_texture);
	CHECK_EXT(GL_WEBGL_compressed_texture_etc1);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_etc1_srgb(void) {
	EXT_FLAG(tex_format.etc1_srgb);

	if(glext.tex_format.etc1) {
		CHECK_EXT(GL_NV_sRGB_formats);
	}

	EXT_MISSING();
}

static void glcommon_ext_texture_format_etc2_eac(void) {
	EXT_FLAG(tex_format.etc2_eac);

	CHECK_CORE(GL_ATLEAST(4, 3) || (GLES_ATLEAST(3, 0) && !glext.version.is_webgl));
	CHECK_EXT(GL_OES_compressed_ETC2_RGBA8_texture);
	CHECK_EXT(GL_ARB_ES3_compatibility);
	CHECK_EXT(GL_ANGLE_compressed_texture_etc);
	CHECK_EXT(GL_WEBGL_compressed_texture_etc);

	// FIXME: maybe don't just assume R11/RG11 are supported as well?

	EXT_MISSING();
}

static void glcommon_ext_texture_format_etc2_eac_srgb(void) {
	EXT_FLAG(tex_format.etc2_eac_srgb);

	CHECK_CORE(GL_ATLEAST(4, 3) || (GLES_ATLEAST(3, 0) && !glext.version.is_webgl));
	CHECK_EXT(GL_OES_compressed_ETC2_sRGB8_alpha8_texture);
	CHECK_EXT(GL_ARB_ES3_compatibility);
	CHECK_EXT(GL_WEBGL_compressed_texture_etc);

	// FIXME: maybe don't just assume R11/RG11 are supported as well?

	EXT_MISSING();
}

static void glcommon_ext_texture_format_bptc(void) {
	EXT_FLAG(tex_format.bptc);

	CHECK_CORE(GL_ATLEAST(4, 2));
	CHECK_EXT(GL_ARB_texture_compression_bptc);
	CHECK_EXT(GL_EXT_texture_compression_bptc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_pvrtc(void) {
	EXT_FLAG(tex_format.pvrtc);

	CHECK_EXT(GL_IMG_texture_compression_pvrtc);
	CHECK_EXT(GL_WEBGL_compressed_texture_pvrtc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_pvrtc2(void) {
	EXT_FLAG(tex_format.pvrtc2);

	CHECK_EXT(GL_IMG_texture_compression_pvrtc2);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_pvrtc_srgb(void) {
	EXT_FLAG(tex_format.pvrtc_srgb);

	CHECK_EXT(GL_EXT_pvrtc_sRGB);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_astc(void) {
	EXT_FLAG(tex_format.astc);

	CHECK_CORE(GLES_ATLEAST(3, 2));
	CHECK_EXT(GL_KHR_texture_compression_astc_ldr);
	CHECK_EXT(GL_OES_texture_compression_astc);
	CHECK_EXT(GL_WEBGL_compressed_texture_astc);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_atc(void) {
	EXT_FLAG(tex_format.atc);

	CHECK_EXT(GL_AMD_compressed_ATC_texture);

	EXT_MISSING();
}

static void glcommon_ext_texture_format_fxt1(void) {
	EXT_FLAG(tex_format.fxt1);

	CHECK_EXT(GL_3DFX_texture_compression_FXT1);

	EXT_MISSING();
}

static void glcommon_ext_texture_storage(void) {
	EXT_FLAG(texture_storage);

	CHECK_CORE(GL_ATLEAST(4, 2));
	CHECK_CORE(GLES_ATLEAST(3, 0));
	CHECK_EXT(GL_ARB_texture_storage);

	EXT_MISSING();
}

static void glcommon_ext_invalidate_subdata(void) {
	EXT_FLAG(invalidate_subdata);

	CHECK_CORE(GL_ATLEAST(4, 3));
	CHECK_EXT(GL_ARB_invalidate_subdata);

	EXT_MISSING();
}

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
	mbdata.message =
		"Looks like you have a broken OpenGL driver.\n"
		"Taisei will probably not work correctly, if at all.\n\n"
		"Starting the game in ANGLE mode should fix the problem, but may introduce slowdown.\n\n"
		"Restart in ANGLE mode now? (If unsure, press YES)";
	mbdata.numbuttons = 3;
	mbdata.buttons = (SDL_MessageBoxButtonData[]) {
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Abort" },
		{ 0, 1, "No" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes (safe)" },
	};

	int mbresult = SDL_ShowMessageBox(&mbdata, &button);

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

static const char *detect_broken_norm16(void) {
	const char *gl_vendor = get_unmasked_property(GL_VENDOR, true);
	const char *gl_renderer = get_unmasked_property(GL_RENDERER, true);

	if(!strcmp(gl_vendor, "Mesa") && !strncmp(gl_renderer, "NV", 2)) {
		return "Normalized 16bpc pixel formats are broken on nouveau";
	}

	return NULL;
}

static void glcommon_check_issues(void) {
	glext.issues.avoid_sampler_uniform_updates = glcommon_check_workaround(
		"avoid sampler uniform updates",
		"TAISEI_GL_WORKAROUND_AVOID_SAMPLER_UNIFORM_UPDATES",
		detect_slow_sampler_update
	);

	glext.issues.disable_norm16 = glcommon_check_workaround(
		"disable normalized 16bpc pixel formats",
		"TAISEI_GL_WORKAROUND_DISABLE_NORM16",
		detect_broken_norm16
	);
}

static inline void (*load_gl_func(const char *name))(void);

void glcommon_check_capabilities(void) {
	const char *glslv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	const char *glv = (const char*)glGetString(GL_VERSION);

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

	if(!glext.version.is_webgl) {
		glext.version.is_webgl = glcommon_check_extension("GL_ANGLE_webgl_compatibility");
	}

	if(glext.version.is_webgl) {
		log_info("WebGL compatibility mode");
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

	if(!GL_ATLEAST(3, 3) && !GLES_ATLEAST(3, 0)) {
		log_warn("Unsupported OpenGL version, expect problems");
	}

	detect_broken_intel_driver();

#ifndef STATIC_GLES3
	if(glcommon_check_extension("GL_ANGLE_request_extension")) {
		union {
			void (*fp);
			PFNGLREQUESTEXTENSIONANGLEPROC glRequestExtensionANGLE;
		} u = { load_gl_func("glRequestExtensionANGLE") };
		assert(u.glRequestExtensionANGLE != NULL);

		const char *src_string = (const char*)glGetString(GL_REQUESTABLE_EXTENSIONS_ANGLE);
		char exts[strlen(src_string) + 1];
		char *extsptr = exts, *ext;
		memcpy(exts, src_string, sizeof(exts));

		log_info("Requestable extensions: %s", src_string);

		while((ext = strtok_r(NULL, " ", &extsptr))) {
			if(!ext || !*ext || !env_get(ext, true)) {
				continue;
			}

			u.glRequestExtensionANGLE(ext);
		}
	}
#endif

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

	glcommon_check_issues();

	glcommon_ext_clear_texture();
	glcommon_ext_color_buffer_float();
	glcommon_ext_debug_output();
	glcommon_ext_depth_texture();
	glcommon_ext_draw_buffers();
	glcommon_ext_float_blend();
	glcommon_ext_instanced_arrays();
	glcommon_ext_internalformat_query2();
	glcommon_ext_invalidate_subdata();
	glcommon_ext_pixel_buffer_object();
	glcommon_ext_seamless_cubemap();
	glcommon_ext_texture_filter_anisotropic();
	glcommon_ext_texture_float();
	glcommon_ext_texture_float_linear();
	glcommon_ext_texture_half_float();
	glcommon_ext_texture_half_float_linear();
	glcommon_ext_texture_norm16();
	glcommon_ext_texture_rg();
	glcommon_ext_texture_storage();
	glcommon_ext_texture_swizzle();
	glcommon_ext_vertex_array_object();
	glcommon_ext_viewport_array();

	glcommon_ext_texture_format_r8_srgb();
	glcommon_ext_texture_format_rg8_srgb();
	glcommon_ext_texture_format_rgb8_rgba8_srgb();
	glcommon_ext_texture_format_s3tc_dx1();
	glcommon_ext_texture_format_s3tc_dx5();
	glcommon_ext_texture_format_s3tc_srgb();
	glcommon_ext_texture_format_rgtc();
	glcommon_ext_texture_format_etc1();
	glcommon_ext_texture_format_etc1_srgb();
	glcommon_ext_texture_format_etc2_eac();
	glcommon_ext_texture_format_etc2_eac_srgb();
	glcommon_ext_texture_format_bptc();
	glcommon_ext_texture_format_pvrtc();
	glcommon_ext_texture_format_pvrtc2();
	glcommon_ext_texture_format_pvrtc_srgb();
	glcommon_ext_texture_format_astc();
	glcommon_ext_texture_format_atc();
	glcommon_ext_texture_format_fxt1();

	glcommon_build_shader_lang_table();
	glcommon_init_texture_formats();
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
	glcommon_free_texture_formats();
}

attr_unused
static inline void (*load_gl_func(const char *name))(void) {
	union {
		void *vp;
		void (*fp)(void);
	} c_sucks;
	c_sucks.vp = SDL_GL_GetProcAddress(name);
	// log_debug("%s: %p", name, c_sucks.vp);
	return c_sucks.fp;
}

void glcommon_load_functions(void) {
#ifndef STATIC_GLES3
	int profile, version;

	if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) < 0) {
		log_fatal("SDL_GL_GetAttribute() failed: %s", SDL_GetError());
	}

	if(profile == SDL_GL_CONTEXT_PROFILE_ES) {
		if(!(version = gladLoadGLES2(load_gl_func))) {
			log_fatal("Failed to load OpenGL ES functions");
		}
	} else {
		if(!(version = gladLoadGL(load_gl_func))) {
			log_fatal("Failed to load OpenGL functions");
		}
	}

	glext.version.major = GLAD_VERSION_MAJOR(version);
	glext.version.minor = GLAD_VERSION_MINOR(version);
	log_debug("GLAD reported OpenGL version %i.%i", glext.version.major, glext.version.minor);

	// GLES has only glClearDepthf
	// Core has only glClearDepth until GL 4.1

	if(!HAVE_GL_FUNC(glClearDepth)) {
		glClearDepth = shim_glClearDepth;
		assert(HAVE_GL_FUNC(glClearDepthf));
	}

	if(!HAVE_GL_FUNC(glClearDepthf)) {
		glClearDepthf = shim_glClearDepthf;
		assert(HAVE_GL_FUNC(glClearDepth));
	}
#else
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	glext.version.major = major;
	glext.version.minor = minor;
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
