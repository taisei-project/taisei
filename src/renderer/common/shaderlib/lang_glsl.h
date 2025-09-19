/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"
#include "memory/arena.h"

#include <SDL3/SDL_iostream.h>

typedef enum GLSLProfile {
	GLSL_PROFILE_NONE,
	GLSL_PROFILE_CORE,
	GLSL_PROFILE_COMPATIBILITY,
	GLSL_PROFILE_ES,
} GLSLProfile;

typedef struct GLSLVersion {
	uint version;
	GLSLProfile profile;
} GLSLVersion;

typedef struct ShaderLangInfoGLSL {
	GLSLVersion version;
} ShaderLangInfoGLSL;

typedef struct ShaderSourceMetaGLSL {
} ShaderSourceMetaGLSL;

typedef SDL_IOStream *(*GLSLSourceOpenCallback)(const char *path, void *userdata);

typedef struct GLSLSourceOptions {
	GLSLVersion version;
	ShaderStage stage;
	bool force_version;
	ShaderMacro *macros;
	GLSLSourceOpenCallback file_open_callback;
	void *file_open_callback_userdata;
} GLSLSourceOptions;

bool glsl_load_source(const char *path, ShaderSource *out, MemArena *arena, const GLSLSourceOptions *options) attr_nonnull(1, 2, 3);
char *glsl_parse_version(const char *str, GLSLVersion *out_version) attr_nonnull(1, 2);
int glsl_format_version(char *buf, size_t bufsize, GLSLVersion version) attr_nonnull(1);
bool glsl_version_supports_instanced_rendering(GLSLVersion v);
