/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"
#include <cglm/cglm.h>

enum MatrixMode { // XXX: Mimicking the gl matrix mode api for easy replacement. But we might just want to add proper functions to modify the non modelview matrices.
	MM_MODELVIEW,
	MM_PROJECTION,
	MM_TEXTURE
};

enum {
	RENDER_MATRIX_STACKSIZE = 32,
};

typedef struct MatrixStack MatrixStack;
struct MatrixStack { // debil stack on the stack
	int head;
	mat4 stack[RENDER_MATRIX_STACKSIZE]; // sure hope none of the two stacks overflows.
};


void matstack_init(MatrixStack *ms);
void matstack_push(MatrixStack *ms);
void matstack_pop(MatrixStack *ms);

typedef struct Renderer Renderer;
struct Renderer {
	MatrixMode mode;
	
	MatrixStack modelview;
	MatrixStack projection;
	MatrixStack texture;
	
	Color color;
	
	bool changed;
};

extern Renderer render;

void render_init();

void render_matrix_mode(Renderer *r, MatrixMode mode);

void render_push(Renderer *r);
void render_pop(Renderer *r);

void render_identity(Renderer *r);
void render_translate(Renderer *r, vec3 offset);
void render_rotate(Renderer *r, float angle, vec3 axis);
void render_rotate_deg(Renderer *r, float angle_degrees, vec3 axis);
void render_scale(Renderer *r, vec3 factors);

void render_color(Renderer *r, Color c);
