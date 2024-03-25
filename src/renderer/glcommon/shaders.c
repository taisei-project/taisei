/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "shaders.h"
#include "util.h"
#include "opengl.h"
#include "rwops/rwops_autobuf.h"

ShaderLangInfoArray glcommon_shader_lang_table = { 0 };

static void add_glsl_version_parsed(GLSLVersion v) {
	dynarray_foreach_elem(&glcommon_shader_lang_table, ShaderLangInfo *lang, {
		if(!memcmp(&lang->glsl.version, &v, sizeof(v))) {
			return;
		}
	});

	dynarray_append(&glcommon_shader_lang_table, {
		.lang = SHLANG_GLSL,
		.glsl.version = v,
	});

	if(v.profile == GLSL_PROFILE_NONE && v.version >= 330) {
		v.profile = GLSL_PROFILE_CORE;
		dynarray_append(&glcommon_shader_lang_table, {
			.lang = SHLANG_GLSL,
			.glsl.version = v,
		});
	}
}

static void add_glsl_version_nonstandard(const char *vstr) {
	// because intel wants to be fucking special again

	char a, b, c;

	if(sscanf(vstr, "%c.%c%c - ", &a, &b, &c) == 3) {
		if(isdigit(a) && isdigit(b) && isdigit(c)) {
			GLSLVersion v = { 0 };
			v.version = (a - '0') * 100 + (b - '0') * 10 + (c - '0');
			v.profile = GLSL_PROFILE_NONE;
			add_glsl_version_parsed(v);
			v.profile = GLSL_PROFILE_CORE;
			add_glsl_version_parsed(v);
			v.profile = GLSL_PROFILE_COMPATIBILITY;
			add_glsl_version_parsed(v);
		}
	}
}

static void add_glsl_version(const char *vstr) {
	if(!*vstr) {
		// Special case: the very first GLSL version doesn't have a version string.
		// We'll just ignore it, it's way too old to be useful anyway.
		return;
	}

	GLSLVersion v;

	if(glsl_parse_version(vstr, &v) == vstr) {
		add_glsl_version_nonstandard(vstr);
		return;
	}

	add_glsl_version_parsed(v);
}

static void glcommon_build_shader_lang_table_fallback(void);
static void glcommon_build_shader_lang_table_finish(void);

void glcommon_build_shader_lang_table(void) {
	dynarray_ensure_capacity(&glcommon_shader_lang_table, 8);

	// NOTE: The ability to query supported GLSL versions was added in GL 4.3,
	// but it's not exposed by any extension. This is pretty silly.

	GLint num_versions = 0;

	if(!glext.version.is_webgl) {
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &num_versions);
	}

	for(int i = 0; i < num_versions; ++i) {
		add_glsl_version((char*)glGetStringi(GL_SHADING_LANGUAGE_VERSION, i));
	}

	if(glcommon_shader_lang_table.num_elements < 1) {
		glcommon_build_shader_lang_table_fallback();
	}

	// TODO: Maybe also detect compatibility profile somehow.

	glcommon_build_shader_lang_table_finish();
}

static void glcommon_build_shader_lang_table_fallback(void) {
	log_warn("Can not reliably determine all supported GLSL versions, resorting to guesswork.");

	struct vtable {
		uint gl;
		GLSLVersion glsl;
	};

	static struct vtable version_map[] = {
		// OpenGL Core
		{ 45, { 450, GLSL_PROFILE_NONE } },
		{ 46, { 460, GLSL_PROFILE_NONE } },
		{ 44, { 440, GLSL_PROFILE_NONE } },
		{ 43, { 430, GLSL_PROFILE_NONE } },
		{ 42, { 420, GLSL_PROFILE_NONE } },
		{ 41, { 410, GLSL_PROFILE_NONE } },
		{ 40, { 400, GLSL_PROFILE_NONE } },
		{ 33, { 330, GLSL_PROFILE_NONE } },
		{ 32, { 150, GLSL_PROFILE_NONE } },
		{ 31, { 140, GLSL_PROFILE_NONE } },
		{ 30, { 130, GLSL_PROFILE_NONE } },
		{ 21, { 120, GLSL_PROFILE_NONE } },
		{ 20, { 110, GLSL_PROFILE_NONE } },

		// OpenGL ES
		{ 32, { 320, GLSL_PROFILE_ES } },
		{ 31, { 310, GLSL_PROFILE_ES } },
		{ 30, { 300, GLSL_PROFILE_ES } },
		// { 20, { 100, GLSL_PROFILE_ES } },
	};

	const char *glslvstr = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	const char *glslvstr_orig = glslvstr;
	static const char gles_prefix[] = "OpenGL ES GLSL ES";
	GLSLProfile profile = GLSL_PROFILE_NONE;
	uint major, minor;

	if(glslvstr == NULL) {
		log_error("Failed to obtain the GLSL version string");
		glcommon_free_shader_lang_table();
		return;
	}

	if(strstartswith(glslvstr, gles_prefix)) {
		glslvstr += strlen(gles_prefix);
		profile = GLSL_PROFILE_ES;
	}

	if(sscanf(glslvstr, " %u.%u", &major, &minor) == 2) {
		GLSLVersion glsl_version = { major * 100 + minor, profile };

		// This one *must* work.
		add_glsl_version_parsed(glsl_version);

		// Let's be a bit optimistic here and just assume that all the older versions are supported (except those commented out).
		// This seems to be the common behavior anyway.

		for(struct vtable *v = version_map; v < version_map + sizeof(version_map)/sizeof(*version_map); ++v) {
			if(v->glsl.version < glsl_version.version && v->glsl.profile == profile) {
				add_glsl_version_parsed(v->glsl);
			}
		}
	} else {
		log_error("Failed to parse GLSL version string: %s", glslvstr_orig);

		// We could try to infer it from the context version, but whatever.
		// If we got here, then we're probably fucked anyway.
		glcommon_free_shader_lang_table();
		return;
	}
}

static void glcommon_build_shader_lang_table_finish(void) {
	if(glcommon_shader_lang_table.num_elements > 0) {
		char *str;
		SDL_RWops *abuf = SDL_RWAutoBuffer((void**)&str, 256);
		SDL_RWprintf(abuf, "Supported GLSL versions: ");
		char vbuf[32];

		dynarray_foreach(&glcommon_shader_lang_table, int i, ShaderLangInfo *lang, {
			assert(lang->lang == SHLANG_GLSL);

			glsl_format_version(vbuf, sizeof(vbuf), lang->glsl.version);

			if(i == 0) {
				SDL_RWprintf(abuf, "%s", vbuf);
			} else {
				SDL_RWprintf(abuf, ", %s", vbuf);
			}
		});

		SDL_WriteU8(abuf, 0);
		log_info("%s", str);
		SDL_RWclose(abuf);

		dynarray_compact(&glcommon_shader_lang_table);
	} else {
		log_error("Can not determine supported GLSL versions. Looks like the OpenGL implementation is non-conformant. Expect nothing to work.");
	}
}

void glcommon_free_shader_lang_table(void) {
	dynarray_free_data(&glcommon_shader_lang_table);
}
