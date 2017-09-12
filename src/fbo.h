/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef FBO_H
#define FBO_H

#include "taiseigl.h"

typedef struct {
	GLuint fbo;
	GLuint tex;
	GLuint depth;

	int nw, nh;
	float scale;
} FBO;

void init_fbo(FBO *fbo, float scale);
void reinit_fbo(FBO *fbo, float scale);
void draw_fbo(FBO *fbo);
void draw_fbo_viewport(FBO *fbo);
void delete_fbo(FBO *fbo);

#endif
