/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <string.h>
#include <stdalign.h>

#include "../api.h"
#include "vertex_buffer.h"

typedef struct VertexAttribTypeInfo {
	GLenum gl_type;
	size_t size;
	size_t alignment;
} VertexAttribTypeInfo;

#define VATYPE(gltype, realtype) { gltype, sizeof(realtype), alignof(realtype) }

static VertexAttribTypeInfo va_type_info[] = {
	[VA_FLOAT]  = VATYPE(GL_FLOAT,          GLfloat),
	[VA_BYTE]   = VATYPE(GL_BYTE,           GLbyte),
	[VA_UBYTE]  = VATYPE(GL_UNSIGNED_BYTE,  GLubyte),
	[VA_SHORT]  = VATYPE(GL_SHORT,          GLshort),
	[VA_USHORT] = VATYPE(GL_UNSIGNED_SHORT, GLushort),
	[VA_INT]    = VATYPE(GL_INT,            GLint),
	[VA_UINT]   = VATYPE(GL_UNSIGNED_INT,   GLuint),
};

static inline attr_pure VertexAttribTypeInfo* typeinfo(VertexAttribType type) {
	uint type_idx = type;
	assert(type_idx < sizeof(va_type_info)/sizeof(VertexAttribTypeInfo));
	return va_type_info + type_idx;
}

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	assert(nattribs > 0);
	assert(attribs != NULL);

	/*
	 * Infer the structure layout, calculate stride and offsets with respect to the alignment requirements.
	 */

	size_t offsets[nattribs];
	size_t stride = 0;

	for(uint i = 0; i < nattribs; ++i) {
		VertexAttribFormat *a = attribs + i;
		VertexAttribTypeInfo *tinfo = typeinfo(a->type);
		assert(a->size > 0 && a->size <= 4);

		size_t sz = tinfo->size;
		size_t al = tinfo->alignment;
		size_t misalign = (al - (stride & (al - 1))) & (al - 1);

		stride += misalign;
		offsets[i] = stride;
		stride += sz * a->size;
	}

	/*
	 * Allocate the VAO and VBO
	 */

	memset(vbuf, 0, sizeof(VertexBuffer));
	vbuf->impl = calloc(1, sizeof(VertexBufferImpl));
	vbuf->size = capacity;

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	glGenVertexArrays(1, &vbuf->impl->gl_vao);
	glGenBuffers(1, &vbuf->impl->gl_vbo);

	r_vertex_buffer(vbuf);
	r_flush();

	glBufferData(GL_ARRAY_BUFFER, capacity, NULL, GL_STREAM_DRAW);

	/*
	 * Emit the attributes.
	 */

	for(uint i = 0; i < nattribs; ++i) {
		VertexAttribFormat *a = attribs + i;
		glEnableVertexAttribArray(i);

		switch(a->coversion) {
			case VA_CONVERT_FLOAT:
			case VA_CONVERT_FLOAT_NORMALIZED:
				glVertexAttribPointer(
					i,
					a->size,
					typeinfo(a->type)->gl_type,
					a->coversion == VA_CONVERT_FLOAT_NORMALIZED,
					stride,
					(void*)offsets[i]
				);
				break;

			case VA_CONVERT_INT:
				break;

			default: UNREACHABLE;
		}
	}

	r_vertex_buffer(vbuf_saved);
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	if(vbuf->impl) {
		glDeleteBuffers(1, &vbuf->impl->gl_vbo);
		glDeleteVertexArrays(1, &vbuf->impl->gl_vao);
		free(vbuf->impl);
		vbuf->impl = NULL;
	}
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	r_vertex_buffer(vbuf);
	r_flush();

	vbuf->offset = 0;
	glBufferData(GL_ARRAY_BUFFER, vbuf->size, NULL, GL_STREAM_DRAW);

	r_vertex_buffer(vbuf_saved);
}

void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) {
	assert(data_size > 0);
	assert(offset + data_size <= vbuf->size);

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	r_vertex_buffer(vbuf);
	r_flush();

	glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data);

	r_vertex_buffer(vbuf_saved);
}

void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	log_debug("%u -> %u / %u", (uint)vbuf->offset, (uint)(vbuf->offset + data_size), (uint)vbuf->size);
	r_vertex_buffer_write(vbuf, vbuf->offset, data_size, data);
	vbuf->offset += data_size;
}

void gl33_vertex_buffer_append_quad_model(VertexBuffer *vbuf) {
	StaticModelVertex verts[] = {
		{ { -0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 0, 0 } },
		{ { -0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 0, 1 } },
		{ {  0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 1, 1 } },
		{ {  0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 1, 0 } },
	};

	r_vertex_buffer_append(vbuf, sizeof(verts), verts);
}
