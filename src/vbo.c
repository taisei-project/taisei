/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "vbo.h"
#include <string.h>
#include "taisei_err.h"

VBO _vbo;

void init_vbo(VBO *vbo, int size) {
	memset(vbo, 0, sizeof(VBO));
	vbo->size = size;

	glGenBuffers(1, &vbo->vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*size, NULL, GL_STATIC_DRAW);

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), NULL);
	glNormalPointer(GL_FLOAT, sizeof(Vertex), (uint8_t*)NULL + sizeof(Vector));
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (uint8_t*)NULL + 2*sizeof(Vector));

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
}

void vbo_add_verts(VBO *vbo, Vertex *verts, int count) {
	if(vbo->offset + count > vbo->size)
		errx(-1, "vbo_add_verts():\n !- Cannot add Vertices: VBO too small!\n");

	glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vertex)*vbo->offset, sizeof(Vertex)*count, verts);

	vbo->offset += count;
}

void init_quadvbo(void) {
	Vertex verts[] = {
		{{-0.5,-0.5,0},{0,0,1},0,0},
		{{-0.5,0.5,0},{0,0,1},0,1},
		{{0.5,0.5,0},{0,0,1},1,1},
		{{0.5,-0.5,0},{0,0,1},1,0},

		// Alternative quad for FBO
		{{-0.5,-0.5,0},{0,0,1},0,1},
		{{-0.5,0.5,0},{0,0,1},0,0},
		{{0.5,0.5,0},{0,0,1},1,0},
		{{0.5,-0.5,0},{0,0,1},1,1}
	};

	init_vbo(&_vbo, VBO_SIZE);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo.vbo);

	vbo_add_verts(&_vbo, verts, 8);
}

void delete_vbo(VBO *vbo) {
	glDeleteBuffers(1, &vbo->vbo);
}

void draw_quad(void) {

// 	glBindBuffer(GL_ARRAY_BUFFER, _vbo.vbo);

	glDrawArrays(GL_QUADS, 0, 4);

// 	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
