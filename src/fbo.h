/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "taiseigl.h"

typedef struct FBO {
	GLuint fbo;
	GLuint tex;
	GLuint depth;

	int nw, nh;
	float scale;
} FBO;

#define FBO_FRONT 0
#define FBO_BACK 1

typedef struct FBOPair {
	/*
	 *  Rule of thumb:
	 *      1. Bind back buffer;
	 *      2. Draw front buffer;
	 *      3. Call swap_fbo_pair;
	 *      4. Rinse, repeat.
	 */

	FBO *front;
	FBO *back;

	struct {
		FBO array[2];
	} _fbopair_private;
} FBOPair;

void init_fbo_pair(FBOPair *pair, float scale, int type);
void delete_fbo_pair(FBOPair *pair);
void swap_fbo_pair(FBOPair *pair);

void draw_fbo(FBO *fbo);
void draw_fbo_viewport(FBO *fbo);
