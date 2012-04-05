/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "vbo.h"
#include <string.h>
#include "taisei_err.h"

static GLuint quadvbo;

/*void init_partbuf(PartBuffer *buf) {
	memset(buf, 0, sizeof(buf));
}

void destroy_partbuf(PartBuffer *buf){
	free(buf->poss);
	free(buf->texcs);
	free(buf->clrs);
}

void partbuf_add_batch(PartBuffer *buf, Matrix m, Vector tex, Color *clr) {
	if(buf->cursor >= buf->size) {
		buf->size += 10;
		buf->poss = realloc(buf->poss, buf->size*sizeof(Matrix));
		buf->texcs = realloc(buf->texcs, buf->size*sizeof(Vector));
		buf->clrs = realloc(buf->clrs, buf->size*sizeof(Color));
	}
		
	memcpy(&buf->poss[buf->cursor], m, sizeof(Matrix));
	memcpy(&buf->texcs[buf->cursor], tex, sizeof(Vector));
	
	if(clr != NULL) {
		memcpy(&buf->clrs[buf->cursor], clr, sizeof(Color));
	} else {
		memset(&buf->clrs[buf->cursor], 0, sizeof(Color));
	}
	
	buf->cursor++;
}

void partbuf_clear(PartBuffer *buf) {
	buf->cursor = 0;
}

void partbuf_draw(PartBuffer *buf, GLuint shader, char *posname, char *tcname, char *clrname) {	
	glUniformMatrix4fv(glGetUniformLocation(shader, posname), buf->cursor, 0, (GLfloat *)buf->poss);
	glUniform3fv(glGetUniformLocation(shader, tcname), buf->cursor, (GLfloat *)buf->texcs);
	glUniform4fv(glGetUniformLocation(shader, clrname), buf->cursor, (GLfloat *)buf->clrs);
	
	glDrawArraysInstanced(GL_QUADS, 0, buf->cursor-1, buf->cursor);	
}

void partbuf_add(PartBuffer *buf, Matrix m, Texture *tex, Color *clr) {
	Vector texscale = {1,1,0};
	
	Matrix m2;
	matscale(m2, m, tex->w, tex->h, tex->gltex);
// 	Vector scale = {tex->w, tex->h, tex->gltex};
			
	partbuf_add_batch(buf, m2, texscale, clr);
}*/

void init_quadvbo() {
	glGenBuffers(1, &quadvbo);
	
	Vector verts[] = {
		{-0.5,-0.5,0}, {0,0,0},
		{-0.5,0.5,0}, {0,1,0},
		{0.5,0.5,0}, {1,1,0},
		{0.5,-0.5,0}, {1,0,0}
	};
	
	glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector)*8, verts, GL_STATIC_DRAW);
	
	glVertexPointer(3, GL_FLOAT, sizeof(float)*6, NULL);
	glTexCoordPointer(3, GL_FLOAT, sizeof(float)*6, NULL + 3*sizeof(float));
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void draw_quad() {
	
	glBindBuffer(GL_ARRAY_BUFFER, quadvbo);	
	
// 	glEnableClientState(GL_VERTEX_ARRAY);
// 	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDrawArrays(GL_QUADS, 0, 4);
		
// 	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
// 	glDisableClientState(GL_VERTEX_ARRAY);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);	
}