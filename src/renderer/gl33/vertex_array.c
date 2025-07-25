/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "vertex_array.h"

#include "../glcommon/debug.h"
#include "gl33.h"
#include "index_buffer.h"
#include "vertex_buffer.h"

static GLenum va_type_to_gl_type[] = {
	[VA_FLOAT]  = GL_FLOAT,
	[VA_BYTE]   = GL_BYTE,
	[VA_UBYTE]  = GL_UNSIGNED_BYTE,
	[VA_SHORT]  = GL_SHORT,
	[VA_USHORT] = GL_UNSIGNED_SHORT,
	[VA_INT]    = GL_INT,
	[VA_UINT]   = GL_UNSIGNED_INT,
};

VertexArray* gl33_vertex_array_create(void) {
	auto varr = ALLOC(VertexArray);
	glGenVertexArrays(1, &varr->gl_handle);
	// A VAO must be bound before it's considered valid.
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glIsVertexArray.xhtml
	// https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsVertexArray.xhtml
	gl33_bind_vao(varr->gl_handle);
	gl33_sync_vao();
	assert(glIsVertexArray(varr->gl_handle));
	snprintf(varr->debug_label, sizeof(varr->debug_label), "VAO #%i", varr->gl_handle);
	return varr;
}

void gl33_vertex_array_destroy(VertexArray *varr) {
	gl33_vertex_array_deleted(varr);
	glDeleteVertexArrays(1, &varr->gl_handle);
	mem_free(varr->attachments);
	mem_free(varr->attribute_layout);
	mem_free(varr);
}

static void gl33_vertex_array_update_layout(VertexArray *varr) {
	GLuint vao_saved = gl33_vao_current();
	GLuint vbo_saved = gl33_buffer_current(GL33_BUFFER_BINDING_ARRAY);

	gl33_bind_vao(varr->gl_handle);

	if(varr->layout_dirty_bits & VAO_INDEX_BIT) {
		gl33_sync_vao();
		if(varr->index_attachment) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, varr->index_attachment->cbuf.gl_handle);
			gl33_index_buffer_on_vao_attach(varr->index_attachment, varr->gl_handle);
		} else {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}

	for(uint i = 0; i < varr->num_attributes; ++i) {
		VertexAttribFormat *a = varr->attribute_layout + i;
		assert((uint)a->spec.type < sizeof(va_type_to_gl_type)/sizeof(GLenum));
		assert(a->attachment < VAO_MAX_BUFFERS);

		if(!(varr->layout_dirty_bits & (1u << i))) {
			continue;
		}

		VertexBuffer *vbuf = varr->attachments[a->attachment];

		if(vbuf == NULL) {
			continue;
		}

		gl33_sync_vao();

		gl33_bind_buffer(GL33_BUFFER_BINDING_ARRAY, vbuf->cbuf.gl_handle);
		gl33_sync_buffer(GL33_BUFFER_BINDING_ARRAY);

		glEnableVertexAttribArray(i);

		switch(a->spec.conversion) {
			case VA_CONVERT_FLOAT:
			case VA_CONVERT_FLOAT_NORMALIZED:
				glVertexAttribPointer(
					i,
					a->spec.elements,
					va_type_to_gl_type[a->spec.type],
					a->spec.conversion == VA_CONVERT_FLOAT_NORMALIZED,
					a->stride,
					(void*)a->offset
				);

				break;

			case VA_CONVERT_INT:
				glVertexAttribIPointer(
					i,
					a->spec.elements,
					va_type_to_gl_type[a->spec.type],
					a->stride,
					(void*)a->offset
				);

				break;

			default: UNREACHABLE;
		}

		DIAGNOSTIC(push)
		DIAGNOSTIC(ignored "-Wunreachable-code")

		if(HAVE_GL_FUNC(glVertexAttribDivisor)) {
			glVertexAttribDivisor(i, a->spec.divisor);
		} else if(a->spec.divisor != 0) {
			log_fatal("Renderer backend does not support instance attributes");
		}

		DIAGNOSTIC(pop)
	}

	for(uint i = varr->num_attributes; i < varr->prev_num_attributes; ++i) {
		gl33_sync_vao();
		glDisableVertexAttribArray(i);
	}

	gl33_bind_buffer(GL33_BUFFER_BINDING_ARRAY, vbo_saved);
	gl33_bind_vao(vao_saved);

	varr->prev_num_attributes = varr->num_attributes;
	varr->layout_dirty_bits = 0;
}

void gl33_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	assert(attachment < VAO_MAX_BUFFERS);

	// TODO: more efficient way of handling this?
	if(attachment >= varr->num_attachments) {
		varr->attachments = mem_realloc(varr->attachments, (attachment + 1) * sizeof(VertexBuffer*));
		varr->num_attachments = attachment + 1;
	}

	varr->attachments[attachment] = vbuf;
	varr->layout_dirty_bits |= (1u << attachment);
}

void gl33_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf) {
	varr->index_attachment = ibuf;
	varr->layout_dirty_bits |= VAO_INDEX_BIT;
}

VertexBuffer* gl33_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment) {
	if(varr->num_attachments <= attachment) {
		return NULL;
	}

	return varr->attachments[attachment];
}

IndexBuffer* gl33_vertex_array_get_index_attachment(VertexArray *varr) {
	return varr->index_attachment;
}

void gl33_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	if(varr->num_attributes != nattribs) {
		varr->attribute_layout = mem_realloc(varr->attribute_layout, sizeof(VertexAttribFormat) * nattribs);
		varr->num_attributes = nattribs;
	}

	memcpy(varr->attribute_layout, attribs, sizeof(VertexAttribFormat) * nattribs);
	varr->layout_dirty_bits |= (1u << nattribs) - 1;
}

const char* gl33_vertex_array_get_debug_label(VertexArray *varr) {
	return varr->debug_label;
}

void gl33_vertex_array_set_debug_label(VertexArray *varr, const char *label) {
	glcommon_set_debug_label(varr->debug_label, "VAO", GL_VERTEX_ARRAY, varr->gl_handle, label);
}

void gl33_vertex_array_flush_buffers(VertexArray *varr) {
	if(varr->layout_dirty_bits) {
		gl33_vertex_array_update_layout(varr);
	}

	for(uint i = 0; i < varr->num_attachments; ++i) {
		if(varr->attachments[i] != NULL) {
			gl33_vertex_buffer_flush(varr->attachments[i]);
		}
	}

	if(varr->index_attachment != NULL) {
		gl33_index_buffer_flush(varr->index_attachment);
	}
}
