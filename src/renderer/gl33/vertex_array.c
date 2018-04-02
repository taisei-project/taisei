/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "vertex_array.h"
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

void r_vertex_array_create(VertexArray *varr) {
	memset(varr, 0, sizeof(*varr));
	varr->impl = calloc(1, sizeof(VertexArrayImpl));
	glGenVertexArrays(1, &varr->impl->gl_handle);
}

void r_vertex_array_destroy(VertexArray *varr) {
	if(varr->impl) {
		gl33_vertex_array_deleted(varr);
		glDeleteVertexArrays(1, &varr->impl->gl_handle);
		free(varr->impl->attachments);
		free(varr->impl->attribute_layout);
		free(varr->impl);
		varr->impl = NULL;
	}
}

static void gl33_vertex_array_update_layout(VertexArray *varr, uint attachment, uint old_num_attribs) {
	GLuint vao_saved = gl33_vao_current();
	GLuint vbo_saved = gl33_vbo_current();

	gl33_bind_vao(varr->impl->gl_handle);
	gl33_sync_vao();

	for(uint i = 0; i < varr->impl->num_attributes; ++i) {
		VertexAttribFormat *a = varr->impl->attribute_layout + i;
		assert((uint)a->spec.type < sizeof(va_type_to_gl_type)/sizeof(GLenum));

		if(attachment != UINT_MAX && attachment != a->attachment) {
			continue;
		}

		VertexBuffer *vbuf = r_vertex_array_get_attachment(varr, a->attachment);

		if(vbuf == NULL) {
			continue;
		}

		assert(vbuf->impl != NULL);

		gl33_bind_vbo(vbuf->impl->gl_handle);
		gl33_sync_vbo();

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

	for(uint i = varr->impl->num_attributes; i < old_num_attribs; ++i) {
		glDisableVertexAttribArray(i);
	}

	gl33_bind_vbo(vbo_saved);
	gl33_bind_vao(vao_saved);
}

void r_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	assert(varr->impl != NULL);

	// TODO: more efficient way of handling this?
	if(attachment >= varr->impl->num_attachments) {
		varr->impl->attachments = realloc(varr->impl->attachments, (attachment + 1) * sizeof(VertexBuffer*));
		varr->impl->num_attachments = attachment + 1;
	}

	varr->impl->attachments[attachment] = vbuf;
	gl33_vertex_array_update_layout(varr, attachment, varr->impl->num_attributes);
}

VertexBuffer* r_vertex_array_get_attachment(VertexArray *varr, uint attachment) {
	assert(varr->impl != NULL);

	if(varr->impl->num_attachments <= attachment) {
		return NULL;
	}

	return varr->impl->attachments[attachment];
}

void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	assert(varr->impl != NULL);

	uint old_nattribs = varr->impl->num_attributes;

	if(varr->impl->num_attributes != nattribs) {
		varr->impl->attribute_layout = realloc(varr->impl->attribute_layout, sizeof(VertexAttribFormat) * nattribs);
		varr->impl->num_attributes = nattribs;
	}

	memcpy(varr->impl->attribute_layout, attribs, sizeof(VertexAttribFormat) * nattribs);
	gl33_vertex_array_update_layout(varr, UINT32_MAX, old_nattribs);
}
