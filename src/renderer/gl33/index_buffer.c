/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "index_buffer.h"

#include "../glcommon/debug.h"
#include "gl33.h"

static void gl33_index_buffer_pre_bind(CommonBuffer *cbuf) {
	IndexBuffer *ibuf = (IndexBuffer*)cbuf;
	ibuf->prev_vao = gl33_vao_current();
	gl33_bind_vao(ibuf->vao);
	gl33_sync_vao();
}

static void gl33_index_buffer_post_bind(CommonBuffer *cbuf) {
	IndexBuffer *ibuf = (IndexBuffer*)cbuf;
	gl33_bind_vao(ibuf->prev_vao);
}

static GLuint index_size_to_datatype(uint size) {
	switch(size) {
		case 2: return GL_UNSIGNED_SHORT;
		case 4: return GL_UNSIGNED_INT;
		default: UNREACHABLE;
	}
}

IndexBuffer *gl33_index_buffer_create(uint index_size, size_t max_elements) {
	IndexBuffer *ibuf = (IndexBuffer*)gl33_buffer_create(GL33_BUFFER_BINDING_ELEMENT_ARRAY, sizeof(IndexBuffer));
	ibuf->index_size = index_size;
	ibuf->index_datatype = index_size_to_datatype(index_size);
	ibuf->cbuf.pre_bind = gl33_index_buffer_pre_bind;
	ibuf->cbuf.post_bind = gl33_index_buffer_post_bind;

	snprintf(ibuf->cbuf.debug_label, sizeof(ibuf->cbuf.debug_label), "IBO #%i", ibuf->cbuf.gl_handle);

	cachedbuf_resize(&ibuf->cbuf.cachedbuf, max_elements * index_size);
	max_elements = ibuf->cbuf.cachedbuf.size / index_size;

	log_debug("Created IBO %u for %zu elements", ibuf->cbuf.gl_handle, max_elements);
	return ibuf;
}

void gl33_index_buffer_on_vao_attach(IndexBuffer *ibuf, GLuint vao) {
	bool initialized = ibuf->vao != 0;
	ibuf->vao = vao;

	if(!initialized) {
		GLenum usage = GL_DYNAMIC_DRAW;  // FIXME
		gl33_buffer_init(&ibuf->cbuf, ibuf->cbuf.cachedbuf.size, NULL, usage);
		glcommon_set_debug_label_gl(GL_BUFFER, ibuf->cbuf.gl_handle, ibuf->cbuf.debug_label);
		log_debug("Initialized %s", ibuf->cbuf.debug_label);
	}
}

size_t gl33_index_buffer_get_capacity(IndexBuffer *ibuf) {
	return ibuf->cbuf.cachedbuf.size / ibuf->index_size;
}

uint gl33_index_buffer_get_index_size(IndexBuffer *ibuf) {
	return ibuf->index_size;
}

const char *gl33_index_buffer_get_debug_label(IndexBuffer *ibuf) {
	return ibuf->cbuf.debug_label;
}

void gl33_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) {
	glcommon_set_debug_label_local(ibuf->cbuf.debug_label, "IBO", ibuf->cbuf.gl_handle, label);

	if(ibuf->vao) {
		glcommon_set_debug_label_gl(GL_BUFFER, ibuf->cbuf.gl_handle, label);
	}
}

void gl33_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) {
	ibuf->cbuf.cachedbuf.stream_offset = offset * ibuf->index_size;
}

size_t gl33_index_buffer_get_offset(IndexBuffer *ibuf) {
	return ibuf->cbuf.cachedbuf.stream_offset / ibuf->index_size;
}

void gl33_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data) {
	SDL_IOStream *stream = gl33_buffer_get_stream(&ibuf->cbuf);
	SDL_WriteIO(stream, data, data_size);
}

void gl33_index_buffer_destroy(IndexBuffer *ibuf) {
	log_debug("Deleted IBO %u", ibuf->cbuf.gl_handle);
	gl33_buffer_destroy(&ibuf->cbuf);
}

void gl33_index_buffer_flush(IndexBuffer *ibuf) {
	gl33_buffer_flush(&ibuf->cbuf);
}

void gl33_index_buffer_invalidate(IndexBuffer *ibuf) {
	gl33_buffer_invalidate(&ibuf->cbuf);
}
