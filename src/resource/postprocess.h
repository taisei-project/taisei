/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
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

	union {
		PostprocessShaderUniformValue *values;
		Texture *texture;
	};

	uint elements;
};

typedef void (*PostprocessDrawFuncPtr)(Framebuffer *fb, double w, double h);
typedef void (*PostprocessPrepareFuncPtr)(Framebuffer *fb, ShaderProgram *prog, void *arg);

PostprocessShader *postprocess_load(const char *path, ResourceFlags flags);
void postprocess_unload(PostprocessShader **list);
void postprocess(PostprocessShader *ppshaders, FBPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw, double width, double height, void *arg);

extern ResourceHandler postprocess_res_handler;

DEFINE_OPTIONAL_RESOURCE_GETTER(PostprocessShader, res_postprocess, RES_POSTPROCESS)
