/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PartBuffer_H
#define PartBuffer_H

#include "matrix.h"
#include "resource/texture.h"
#include "resource/shader.h"
#include <GL/glew.h>

/* PartBuffer for fast instanced Particle/whatever (maybe everything) rendering (for now rocket science)*/
/*
typedef struct {
	Matrix *poss;
	Vector *texcs;
	Color *clrs;
	
	int cursor;
	int size;
} PartBuffer;


void init_partbuf(PartBuffer *buf);
void destroy_partbuf(PartBuffer *buf);

void partbuf_add_batch(PartBuffer *buf, Matrix m, Vector v, Color *clr);
void partbuf_clear(PartBuffer *buf);
void partbuf_draw(PartBuffer *buf, GLuint shader, char *posname, char *tcname, char *clrname);
void partbuf_add(PartBuffer *buf, Matrix m, Texture *tex, Color *clr);
*/

extern GLuint _quadvbo;

void init_quadvbo();
void draw_quad();
void delete_vbo(GLuint *vbo);

#endif