/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../common/opengl.h"

typedef struct VertexBufferImpl {
	GLuint gl_vbo;
	GLuint gl_vao;
} VertexBufferImpl;

void gl33_vertex_buffer_append_quad_model(VertexBuffer *vbuf);
