/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef VBO_H
#define VBO_H

#include "matrix.h"
#include "resource/texture.h"
#include "resource/shader.h"
#include "taiseigl.h"

enum {
	VBO_SIZE = 8192, // * sizeof(Vertex)
};

typedef struct VBO VBO;
struct VBO {
	GLuint vbo;
	int offset;
	int size;
};

extern VBO _vbo;

typedef struct Vertex Vertex;
struct Vertex {
	Vector x;
	Vector n;
	float s;
	float t;
};

void init_vbo(VBO *vbo, int size);
void vbo_add_verts(VBO *vbo, Vertex *verts, int count);

void init_quadvbo(void);
void draw_quad(void);
void delete_vbo(VBO *vbo);

#endif
