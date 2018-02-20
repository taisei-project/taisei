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

typedef struct PostprocessShader PostprocessShader;
typedef struct PostprocessShaderUniform PostprocessShaderUniform;

struct PostprocessShader {
	LIST_INTERFACE(PostprocessShader);

	PostprocessShaderUniform *uniforms;
	ShaderProgram *shader;
};

typedef enum PostprocessShaderUniformType {
	PSU_FLOAT,
	PSU_INT,
	PSU_UINT,
} PostprocessShaderUniformType;

typedef union PostprocessShaderUniformValue {
	void *v;
	GLfloat *f;
	GLint *i;
	GLuint *u;
} PostprocessShaderUniformValuePtr;

typedef void (APIENTRY *PostprocessShaderUniformFuncPtr)(GLint, GLsizei, const GLvoid*);

struct PostprocessShaderUniform {
	LIST_INTERFACE(PostprocessShaderUniform);

	PostprocessShaderUniformType type;
	PostprocessShaderUniformValuePtr values;
	PostprocessShaderUniformFuncPtr func;

	int loc;
	int size;
	int amount;
};

typedef void (*PostprocessDrawFuncPtr)(FBO*);
typedef void (*PostprocessPrepareFuncPtr)(FBO*, ShaderProgram*);

char* postprocess_path(const char *path);

PostprocessShader* postprocess_load(const char *path, unsigned int flags);
void postprocess_unload(PostprocessShader **list);
void postprocess(PostprocessShader *ppshaders, FBOPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw);

/*
 *  Glue for resources api
 */

char* postprocess_path(const char *name);
bool check_postprocess_path(const char *path);
void* load_postprocess_begin(const char *path, unsigned int flags);
void* load_postprocess_end(void *opaque, const char *path, unsigned int flags);
void unload_postprocess(void*);

extern ResourceHandler postprocess_res_handler;

#define PP_PATH_PREFIX SHPROG_PATH_PREFIX
#define PP_EXTENSION ".pp"
