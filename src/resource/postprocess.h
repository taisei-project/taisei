/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_postprocess_h
#define IGUARD_resource_postprocess_h

#include "taisei.h"

#include "resource.h"
#include "shader_program.h"
#include "renderer/api.h"
#include "util/graphics.h"

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

typedef void (*PostprocessDrawFuncPtr)(Framebuffer* fb, double w, double h);
typedef void (*PostprocessPrepareFuncPtr)(Framebuffer* fb, ShaderProgram *prog);

char* postprocess_path(const char *path);

PostprocessShader* postprocess_load(const char *path, uint flags);
void postprocess_unload(PostprocessShader **list);
void postprocess(PostprocessShader *ppshaders, FBPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw, double width, double height);

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

#endif // IGUARD_resource_postprocess_h
