/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "list.h"
#include "hashtable.h"
#include "shader.h"
#include "fbo.h"

typedef struct PostprocessShader PostprocessShader;
typedef struct PostprocessShaderUniform PostprocessShaderUniform;

struct PostprocessShader {
    PostprocessShader *next;
    PostprocessShader *prev;
    PostprocessShaderUniform *uniforms;
    Shader *shader;
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
    PostprocessShaderUniform *next;
    PostprocessShaderUniform *prev;
    PostprocessShaderUniformType type;
    PostprocessShaderUniformValuePtr values;
    PostprocessShaderUniformFuncPtr func;
    int loc;
    int size;
    int amount;
};

typedef void (*PostprocessDrawFuncPtr)(FBO*);
typedef void (*PostprocessPrepareFuncPtr)(FBO*, Shader*);

PostprocessShader* postprocess_load(const char *path);
void postprocess_unload(PostprocessShader **list);
FBO* postprocess(PostprocessShader *ppshaders, FBO *fbo1, FBO *fbo2, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw);


