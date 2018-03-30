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
#include "core.h"

static GLenum va_type_to_gl_type[] = {
	[VA_FLOAT]  = GL_FLOAT,
	[VA_BYTE]   = GL_BYTE,
	[VA_UBYTE]  = GL_UNSIGNED_BYTE,
	[VA_SHORT]  = GL_SHORT,
	[VA_USHORT] = GL_UNSIGNED_SHORT,
	[VA_INT]    = GL_INT,
	[VA_UINT]   = GL_UNSIGNED_INT,
};

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	assert(nattribs > 0);
	assert(attribs != NULL);

	memset(vbuf, 0, sizeof(VertexBuffer));
	vbuf->impl = calloc(1, sizeof(VertexBufferImpl));
	vbuf->size = capacity;

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	glGenVertexArrays(1, &vbuf->impl->gl_vao);
	glGenBuffers(1, &vbuf->impl->gl_vbo);

	r_vertex_buffer(vbuf);
	gl33_sync_vertex_buffer();

	glBufferData(GL_ARRAY_BUFFER, capacity, NULL, GL_STATIC_DRAW);

	for(uint i = 0; i < nattribs; ++i) {
		VertexAttribFormat *a = attribs + i;
		assert((uint)a->spec.type < sizeof(va_type_to_gl_type)/sizeof(GLenum));

		glEnableVertexAttribArray(i);

		switch(a->spec.coversion) {
			case VA_CONVERT_FLOAT:
			case VA_CONVERT_FLOAT_NORMALIZED:
				glVertexAttribPointer(
					i,
					a->spec.elements,
					va_type_to_gl_type[a->spec.type],
					a->spec.coversion == VA_CONVERT_FLOAT_NORMALIZED,
					a->stride,
					(void*)a->offset
				);
				glVertexAttribDivisor(i, a->spec.divisor);
				break;

			case VA_CONVERT_INT:
				glVertexAttribIPointer(
					i,
					a->spec.elements,
					va_type_to_gl_type[a->spec.type],
					a->stride,
					(void*)a->offset
				);
				glVertexAttribDivisor(i, a->spec.divisor);
				break;

			default: UNREACHABLE;
		}
	}

	if(vbuf_saved != NULL) {
		r_vertex_buffer(vbuf_saved);
	}
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	if(vbuf->impl) {
		gl33_vertex_buffer_deleted(vbuf);
		glDeleteBuffers(1, &vbuf->impl->gl_vbo);
		glDeleteVertexArrays(1, &vbuf->impl->gl_vao);
		free(vbuf->impl);
		vbuf->impl = NULL;
	}
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	r_vertex_buffer(vbuf);
	gl33_sync_vertex_buffer();

	vbuf->offset = 0;
	glBufferData(GL_ARRAY_BUFFER, vbuf->size, NULL, GL_DYNAMIC_DRAW);

	r_vertex_buffer(vbuf_saved);
}

void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) {
	assert(data_size > 0);
	assert(offset + data_size <= vbuf->size);

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();

	r_vertex_buffer(vbuf);
	gl33_sync_vertex_buffer();

	glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data);

	r_vertex_buffer(vbuf_saved);
}

void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	// log_debug("%u -> %u / %u", (uint)vbuf->offset, (uint)(vbuf->offset + data_size), (uint)vbuf->size);
	r_vertex_buffer_write(vbuf, vbuf->offset, data_size, data);
	vbuf->offset += data_size;
}
