/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "shader_program.h"
#include "fbo.h"
#include "renderer/api.h"

typedef struct PostprocessShader PostprocessShader;
typedef struct PostprocessShaderUniform PostprocessShaderUniform;
typedef union PostprocessShaderUniformValue PostprocessShaderUniformValue;

struct PostprocessShader {
	LIST_INTERFACE(PostprocessShader);

	PostprocessShaderUniform *uniforms;
	ShaderProgram *shader;
};

union PostprocessShaderUniformValue {
	int i;
	float f;
};

struct PostprocessShaderUniform {
	LIST_INTERFACE(PostprocessShaderUniform);

	Uniform *uniform;
	PostprocessShaderUniformValue *values;
	uint elements;
};

typedef void (*PostprocessDrawFuncPtr)(FBO*);
typedef void (*PostprocessPrepareFuncPtr)(FBO*, ShaderProgram*);

char* postprocess_path(const char *path);

PostprocessShader* postprocess_load(const char *path, uint flags);
void postprocess_unload(PostprocessShader **list);
void postprocess(PostprocessShader *ppshaders, FBOPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw);

/*
 *  Glue for resources api
 */

char* postprocess_path(const char *name);
bool check_postprocess_path(const char *path);
void* load_postprocess_begin(const char *path, uint flags);
void* load_postprocess_end(void *opaque, const char *path, uint flags);
void unload_postprocess(void*);

extern ResourceHandler postprocess_res_handler;

#define PP_PATH_PREFIX SHPROG_PATH_PREFIX
#define PP_EXTENSION ".pp"
