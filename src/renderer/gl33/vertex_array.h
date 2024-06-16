/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "opengl.h"

#define VAO_MAX_BUFFERS 31
#define VAO_INDEX_BIT (1u << VAO_MAX_BUFFERS)

struct VertexArray {
	VertexBuffer **attachments;
	VertexAttribFormat *attribute_layout;
	IndexBuffer *index_attachment;
	GLuint gl_handle;
	uint num_attachments;
	uint num_attributes;
	uint prev_num_attributes;
	uint layout_dirty_bits;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

VertexArray* gl33_vertex_array_create(void);
const char* gl33_vertex_array_get_debug_label(VertexArray *varr);
void gl33_vertex_array_set_debug_label(VertexArray *varr, const char *label);
void gl33_vertex_array_destroy(VertexArray *varr);
void gl33_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
void gl33_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf);
VertexBuffer* gl33_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment);
IndexBuffer* gl33_vertex_array_get_index_attachment(VertexArray *varr);
void gl33_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
void gl33_vertex_array_flush_buffers(VertexArray *varr);
