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

typedef struct VertexBuffer {
	size_t size;
	size_t offset;
	GLuint gl_handle;
	char debug_label[R_DEBUG_LABEL_SIZE];
} VertexBuffer;

VertexBuffer* gl33_vertex_buffer_create(size_t capacity, void *data);
const char* gl33_vertex_buffer_get_debug_label(VertexBuffer *vbuf);
void gl33_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label);
void gl33_vertex_buffer_destroy(VertexBuffer *vbuf);
void gl33_vertex_buffer_invalidate(VertexBuffer *vbuf);
void gl33_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data);
void gl33_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data);
size_t gl33_vertex_buffer_get_capacity(VertexBuffer *vbuf);
size_t gl33_vertex_buffer_get_cursor(VertexBuffer *vbuf);
void gl33_vertex_buffer_set_cursor(VertexBuffer *vbuf, size_t position);
