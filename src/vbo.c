/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "vbo.h"
#include <string.h>
#include "log.h"

VBO _vbo;

void init_vbo(VBO *vbo, int size) {
	memset(vbo, 0, sizeof(VBO));
	vbo->size = size;

	GLuint VertexArrayID; // TODO move me to the right place.
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	
	glGenBuffers(1, &vbo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*size, NULL, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); // vertex
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float))); // normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float))); // texture coord

	//glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void vbo_add_verts(VBO *vbo, Vertex *verts, int count) {
	if(vbo->offset + count > vbo->size)
		log_fatal("Cannot add Vertices: VBO too small!");

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

//  glBindBuffer(GL_ARRAY_BUFFER, _vbo.vbo);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

//  glBindBuffer(GL_ARRAY_BUFFER, 0);
}
