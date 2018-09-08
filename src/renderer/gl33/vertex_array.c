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
#include "../glcommon/debug.h"

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
	VertexArray *varr = calloc(1, sizeof(VertexArray));
	glGenVertexArrays(1, &varr->gl_handle);
	snprintf(varr->debug_label, sizeof(varr->debug_label), "VAO #%i", varr->gl_handle);
	return varr;
}

void gl33_vertex_array_destroy(VertexArray *varr) {
	gl33_vertex_array_deleted(varr);
	glDeleteVertexArrays(1, &varr->gl_handle);
	free(varr->attachments);
	free(varr->attribute_layout);
	free(varr);
}

static void gl33_vertex_array_update_layout(VertexArray *varr, uint attachment, uint old_num_attribs) {
	GLuint vao_saved = gl33_vao_current();
	GLuint vbo_saved = gl33_vbo_current();

	gl33_bind_vao(varr->gl_handle);
	gl33_sync_vao();

	for(uint i = 0; i < varr->num_attributes; ++i) {
		VertexAttribFormat *a = varr->attribute_layout + i;
		assert((uint)a->spec.type < sizeof(va_type_to_gl_type)/sizeof(GLenum));

		if(attachment != UINT_MAX && attachment != a->attachment) {
			continue;
		}

		VertexBuffer *vbuf = r_vertex_array_get_attachment(varr, a->attachment);

		if(vbuf == NULL) {
			continue;
		}

		gl33_bind_vbo(vbuf->gl_handle);
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

		if(glVertexAttribDivisor != NULL) {
			glVertexAttribDivisor(i, a->spec.divisor);
		} else if(a->spec.divisor != 0) {
			log_fatal("Renderer backend does not support instance attributes");
		}
	}

	for(uint i = varr->num_attributes; i < old_num_attribs; ++i) {
		glDisableVertexAttribArray(i);
	}

	gl33_bind_vbo(vbo_saved);
	gl33_bind_vao(vao_saved);
}

void gl33_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	// TODO: more efficient way of handling this?
	if(attachment >= varr->num_attachments) {
		varr->attachments = realloc(varr->attachments, (attachment + 1) * sizeof(VertexBuffer*));
		varr->num_attachments = attachment + 1;
	}

	varr->attachments[attachment] = vbuf;
	gl33_vertex_array_update_layout(varr, attachment, varr->num_attributes);
}

VertexBuffer* gl33_vertex_array_get_attachment(VertexArray *varr, uint attachment) {
	if(varr->num_attachments <= attachment) {
		return NULL;
	}

	return varr->attachments[attachment];
}

void gl33_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	uint old_nattribs = varr->num_attributes;

	if(varr->num_attributes != nattribs) {
		varr->attribute_layout = realloc(varr->attribute_layout, sizeof(VertexAttribFormat) * nattribs);
		varr->num_attributes = nattribs;
	}

	memcpy(varr->attribute_layout, attribs, sizeof(VertexAttribFormat) * nattribs);
	gl33_vertex_array_update_layout(varr, UINT32_MAX, old_nattribs);
}

const char* gl33_vertex_array_get_debug_label(VertexArray *varr) {
	return varr->debug_label;
}

void gl33_vertex_array_set_debug_label(VertexArray *varr, const char *label) {
	if(label) {
		log_debug("\"%s\" renamed to \"%s\"", varr->debug_label, label);
		strlcpy(varr->debug_label, label, sizeof(varr->debug_label));
	} else {
		char tmp[sizeof(varr->debug_label)];
		snprintf(tmp, sizeof(tmp), "VAO #%i", varr->gl_handle);
		log_debug("\"%s\" renamed to \"%s\"", varr->debug_label, tmp);
		strlcpy(varr->debug_label, tmp, sizeof(varr->debug_label));
	}

	glcommon_debug_object_label(GL_VERTEX_ARRAY, varr->gl_handle, varr->debug_label);
}
