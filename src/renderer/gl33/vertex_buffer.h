/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "opengl.h"

typedef struct VertexBufferImpl {
	GLuint gl_handle;
} VertexBufferImpl;

void gl33_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data);
void gl33_vertex_buffer_destroy(VertexBuffer *vbuf);
void gl33_vertex_buffer_invalidate(VertexBuffer *vbuf);
void gl33_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data);
void gl33_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data);
