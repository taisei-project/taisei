/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"

typedef RenderTarget FBO;

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
		FBO targets[2];
	} _private;
} FBOPair;

void init_fbo_pair(FBOPair *pair, float scale, TextureType type);
void delete_fbo_pair(FBOPair *pair);
void swap_fbo_pair(FBOPair *pair);

void draw_fbo(FBO *fbo);
void draw_fbo_viewport(FBO *fbo);

// TODO: rename and move this
typedef struct Resources {
	struct {
		FBOPair bg;
		FBOPair fg;
		FBOPair rgba;
	} fbo_pairs;
} Resources;

extern Resources resources;
