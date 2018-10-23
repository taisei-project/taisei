/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "index_buffer.h"
#include "gl33.h"
#include "../glcommon/debug.h"

IndexBuffer* gl33_index_buffer_create(size_t max_elements) {
	IndexBuffer *ibuf = (IndexBuffer*)gl33_buffer_create(
        GL33_BUFFER_BINDING_COPY_WRITE,
        max_elements * sizeof(gl33_ibo_index_t),
        GL_STATIC_DRAW,
        NULL
    );

	snprintf(ibuf->cbuf.debug_label, sizeof(ibuf->cbuf.debug_label), "IBO #%i", ibuf->cbuf.gl_handle);
	log_debug("Created IBO %u for %zu elements", ibuf->cbuf.gl_handle, max_elements);
	return ibuf;
}

size_t gl33_index_buffer_get_capacity(IndexBuffer *ibuf) {
	return ibuf->cbuf.size / sizeof(gl33_ibo_index_t);
}

const char* gl33_index_buffer_get_debug_label(IndexBuffer *ibuf) {
	return ibuf->cbuf.debug_label;
}

void gl33_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) {
	glcommon_set_debug_label(ibuf->cbuf.debug_label, "IBO", GL_BUFFER, ibuf->cbuf.gl_handle, label);
}

void gl33_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) {
	ibuf->cbuf.offset = offset * sizeof(gl33_ibo_index_t);
}

size_t gl33_index_buffer_get_offset(IndexBuffer *ibuf) {
	return ibuf->cbuf.offset / sizeof(gl33_ibo_index_t);
}

void gl33_index_buffer_add_indices(IndexBuffer *ibuf, uint index_ofs, size_t num_elements, uint indices[num_elements]) {
	SDL_RWops *stream = gl33_buffer_get_stream(&ibuf->cbuf);
	gl33_ibo_index_t data[num_elements];

	for(size_t i = 0; i < num_elements; ++i) {
		uintmax_t idx = indices[i] + index_ofs;
		assert(idx <= GL33_IBO_MAX_INDEX);
		data[i] = idx;
	}

	SDL_RWwrite(stream, &data, sizeof(gl33_ibo_index_t), num_elements);
}

void gl33_index_buffer_destroy(IndexBuffer *ibuf) {
	log_debug("Deleted IBO %u", ibuf->cbuf.gl_handle);
	gl33_buffer_destroy(&ibuf->cbuf);
}

void gl33_index_buffer_flush(IndexBuffer *ibuf) {
	gl33_buffer_flush(&ibuf->cbuf);
}
