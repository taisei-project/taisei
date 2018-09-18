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

#define STREAM_VBUF(rw) ((VertexBuffer*)rw)

static int64_t gl33_vertex_buffer_stream_seek(SDL_RWops *rw, int64_t offset, int whence) {
	VertexBuffer *vbuf = STREAM_VBUF(rw);

	switch(whence) {
		case RW_SEEK_CUR: {
			vbuf->offset += offset;
			break;
		}

		case RW_SEEK_END: {
			vbuf->offset = vbuf->size + offset;
			break;
		}

		case RW_SEEK_SET: {
			vbuf->offset = offset;
			break;
		}
	}

	assert(vbuf->offset < vbuf->size);
	return vbuf->offset;
}

static int64_t gl33_vertex_buffer_stream_size(SDL_RWops *rw) {
	VertexBuffer *vbuf = STREAM_VBUF(rw);
	return vbuf->size;
}

static size_t gl33_vertex_buffer_stream_write(SDL_RWops *rw, const void *data, size_t size, size_t num) {
	VertexBuffer *vbuf = STREAM_VBUF(rw);
	size_t total_size = size * num;
	assert(vbuf->offset + total_size <= vbuf->size);

	if(total_size > 0) {
		GLuint vbo_saved = gl33_vbo_current();
		gl33_bind_vbo(vbuf->gl_handle);
		gl33_sync_vbo();
		glBufferSubData(GL_ARRAY_BUFFER, vbuf->offset, total_size, data);
		gl33_bind_vbo(vbo_saved);
		vbuf->offset += total_size;
	}

	return num;
}

static size_t gl33_vertex_buffer_stream_read(SDL_RWops *rw, void *data, size_t size, size_t num) {
	SDL_SetError("Stream is write-only");
	return 0;
}

static int gl33_vertex_buffer_stream_close(SDL_RWops *rw) {
	SDL_SetError("Can't close a vertex buffer stream");
	return -1;
}

SDL_RWops* gl33_vertex_buffer_get_stream(VertexBuffer *vbuf) {
	return &vbuf->stream;
}

VertexBuffer* gl33_vertex_buffer_create(size_t capacity, void *data) {
	VertexBuffer *vbuf = calloc(1, sizeof(VertexBuffer));
	vbuf->size = capacity = topow2(capacity);
	glGenBuffers(1, &vbuf->gl_handle);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->gl_handle);
	gl33_sync_vbo();
	assert(glIsBuffer(vbuf->gl_handle));
	glBufferData(GL_ARRAY_BUFFER, capacity, data, GL_STATIC_DRAW);
	gl33_bind_vbo(vbo_saved);

	vbuf->stream.type = SDL_RWOPS_UNKNOWN;
	vbuf->stream.close = gl33_vertex_buffer_stream_close;
	vbuf->stream.read = gl33_vertex_buffer_stream_read;
	vbuf->stream.write = gl33_vertex_buffer_stream_write;
	vbuf->stream.seek = gl33_vertex_buffer_stream_seek;
	vbuf->stream.size = gl33_vertex_buffer_stream_size;

	snprintf(vbuf->debug_label, sizeof(vbuf->debug_label), "VBO #%i", vbuf->gl_handle);
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

const char* gl33_vertex_buffer_get_debug_label(VertexBuffer *vbuf) {
	return vbuf->debug_label;
}

void gl33_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label) {
	glcommon_set_debug_label(vbuf->debug_label, "VBO", GL_BUFFER, vbuf->gl_handle, label);
}

void gl33_vertex_buffer_flush(VertexBuffer *vbuf) {

}
