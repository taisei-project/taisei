/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource/shader_program.h"
#include "color.h"

typedef enum MatrixMode {
	// XXX: Mimicking the gl matrix mode api for easy replacement. But we might just want to add proper functions to modify the non modelview matrices.
	MM_MODELVIEW,
	MM_PROJECTION,
	MM_TEXTURE,
	MM_NUM_MODES,
} MatrixMode;

enum {
	RENDER_MATRIX_STACKSIZE = 32,
};

typedef struct MatrixStack { // debil stack on the stack
	mat4 *head;

	// the alignment is required for the SSE codepath in CGLM
	mat4 stack[RENDER_MATRIX_STACKSIZE] CGLM_ALIGN(32);
} MatrixStack;

void matstack_init(MatrixStack *ms);
void matstack_push(MatrixStack *ms);
void matstack_pop(MatrixStack *ms);

typedef struct RendererUBOData {
	mat4 modelview;
	mat4 projection;
	mat4 texture;
	vec4 color;
} RendererUBOData;

void render_init(void);
void render_free(void);

void render_matrix_mode(MatrixMode mode);

void render_push(void);
void render_pop(void);

void render_identity(void);
void render_translate(float x, float y, float z);
void render_rotate(float angle, float nx, float ny, float nz);
void render_rotate_deg(float angle_degrees, float nx, float ny, float nz);
void render_scale(float sx, float sy, float sz);

void render_ortho(float left, float right, float bottom, float top, float near, float far);
void render_perspective(float angle, float aspect, float near, float far);

void render_color(Color c);
void render_color4(float r, float g, float b, float a);
void render_color3(float r, float g, float b);

void render_shader(ShaderProgram *prog);
void render_shader_standard(void);
void render_shader_standard_notex(void);

ShaderProgram* render_current_shader(void);

void render_draw_quad(void);

#define R_SCOPE(code) do { \
	render_push(); \
	do { \
		code \
	} while(0); \
	render_pop(); \
} while(0)
