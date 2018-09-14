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
#include "../glcommon/debug.h"

VertexBuffer* gl33_vertex_buffer_create(size_t capacity, void *data) {
	VertexBuffer *vbuf = calloc(1, sizeof(VertexBuffer));
	vbuf->size = capacity = topow2(capacity);
	glGenBuffers(1, &vbuf->gl_handle);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->gl_handle);
	gl33_sync_vbo();
	glBufferData(GL_ARRAY_BUFFER, capacity, data, GL_STATIC_DRAW);
	gl33_bind_vbo(vbo_saved);

	log_debug("Created VBO %u with %zukb of storage", vbuf->gl_handle, vbuf->size / 1024);
	return vbuf;
}

void gl33_vertex_buffer_destroy(VertexBuffer *vbuf) {
	gl33_vertex_buffer_deleted(vbuf);
	glDeleteBuffers(1, &vbuf->gl_handle);
	log_debug("Deleted VBO %u with %zukb of storage", vbuf->gl_handle, vbuf->size / 1024);
	free(vbuf);
}

void gl33_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->gl_handle);
	gl33_sync_vbo();
	glBufferData(GL_ARRAY_BUFFER, vbuf->size, NULL, GL_DYNAMIC_DRAW);
	gl33_bind_vbo(vbo_saved);
	vbuf->offset = 0;
}

void gl33_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) {
	assert(data_size > 0);
	assert(offset + data_size <= vbuf->size);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->gl_handle);
	gl33_sync_vbo();
	glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data);
	gl33_bind_vbo(vbo_saved);
}

void gl33_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	// log_debug("%u -> %u / %u", (uint)vbuf->offset, (uint)(vbuf->offset + data_size), (uint)vbuf->size);
	r_vertex_buffer_write(vbuf, vbuf->offset, data_size, data);
	vbuf->offset += data_size;
}

size_t gl33_vertex_buffer_get_capacity(VertexBuffer *vbuf) {
	return vbuf->size;
}

size_t gl33_vertex_buffer_get_cursor(VertexBuffer *vbuf) {
	return vbuf->offset;
}

void gl33_vertex_buffer_set_cursor(VertexBuffer *vbuf, size_t position) {
	assert(position < vbuf->size);
	vbuf->offset = position;
}

const char* gl33_vertex_buffer_get_debug_label(VertexBuffer *vbuf) {
	return vbuf->debug_label;
}

void gl33_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label) {
	glcommon_set_debug_label(vbuf->debug_label, "VBO", GL_BUFFER, vbuf->gl_handle, label);
}
