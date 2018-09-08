/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "opengl.h"

struct VertexArray {
	VertexBuffer **attachments;
	VertexAttribFormat *attribute_layout;
	GLuint gl_handle;
	uint num_attachments;
	uint num_attributes;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

VertexArray* gl33_vertex_array_create(void);
const char* gl33_vertex_array_get_debug_label(VertexArray *varr);
void gl33_vertex_array_set_debug_label(VertexArray *varr, const char *label);
void gl33_vertex_array_destroy(VertexArray *varr);
void gl33_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
VertexBuffer* gl33_vertex_array_get_attachment(VertexArray *varr, uint attachment);
void gl33_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
