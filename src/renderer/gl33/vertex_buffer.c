/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "vertex_buffer.h"

#include "../glcommon/debug.h"
#include "gl33.h"

VertexBuffer* gl33_vertex_buffer_create(size_t capacity, void *data) {
	VertexBuffer *vbuf = (VertexBuffer*)gl33_buffer_create(GL33_BUFFER_BINDING_ARRAY, sizeof(VertexBuffer));
	gl33_buffer_init(&vbuf->cbuf, capacity, data, GL_STATIC_DRAW);

	snprintf(vbuf->cbuf.debug_label, sizeof(vbuf->cbuf.debug_label), "VBO #%i", vbuf->cbuf.gl_handle);
	log_debug("Created VBO %u with %zukb of storage", vbuf->cbuf.gl_handle, vbuf->cbuf.size / 1024);
	return vbuf;
}

void gl33_vertex_buffer_destroy(VertexBuffer *vbuf) {
	log_debug("Deleted VBO %u with %zukb of storage", vbuf->cbuf.gl_handle, vbuf->cbuf.size / 1024);
	gl33_buffer_destroy(&vbuf->cbuf);
}

void gl33_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	gl33_buffer_invalidate(&vbuf->cbuf);
}

const char* gl33_vertex_buffer_get_debug_label(VertexBuffer *vbuf) {
	return vbuf->cbuf.debug_label;
}

void gl33_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label) {
	glcommon_set_debug_label(vbuf->cbuf.debug_label, "VBO", GL_BUFFER, vbuf->cbuf.gl_handle, label);
}

void gl33_vertex_buffer_flush(VertexBuffer *vbuf) {
	gl33_buffer_flush(&vbuf->cbuf);
}

SDL_RWops* gl33_vertex_buffer_get_stream(VertexBuffer *vbuf) {
	return gl33_buffer_get_stream(&vbuf->cbuf);
}
