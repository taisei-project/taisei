/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

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

#include "shader.h"

typedef struct GLSLMacro {
	const char *name;
	const char *value;
} GLSLMacro;

typedef struct GLSLSourceOptions {
	GLSLVersion version;
	ShaderStage stage;
	bool force_version;
	GLSLMacro *macros;
} GLSLSourceOptions;

bool glsl_load_source(const char *path, ShaderSource *out, const GLSLSourceOptions *options) attr_nonnull(1, 2, 3);
