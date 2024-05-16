/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "index_buffer.h"

IndexBuffer *gles20_index_buffer_create(uint index_size, size_t max_elements) {
	assert(index_size == sizeof(gles20_ibo_index_t));
	auto ibuf = ALLOC_FLEX(IndexBuffer, max_elements * sizeof(gles20_ibo_index_t));
	snprintf(ibuf->debug_label, sizeof(ibuf->debug_label), "Fake IBO at %p", (void*)ibuf);
	ibuf->num_elements = max_elements;
	return ibuf;
}

size_t gles20_index_buffer_get_capacity(IndexBuffer *ibuf) {
	return ibuf->num_elements;
}

uint gles20_index_buffer_get_index_size(IndexBuffer *ibuf) {
	return sizeof(gles20_ibo_index_t);
}

const char *gles20_index_buffer_get_debug_label(IndexBuffer *ibuf) {
	return ibuf->debug_label;
}

void gles20_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) {
	if(label) {
		strlcpy(ibuf->debug_label, label, sizeof(ibuf->debug_label));
	} else {
		snprintf(ibuf->debug_label, sizeof(ibuf->debug_label), "Fake IBO at %p", (void*)ibuf);
	}
}

void gles20_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) {
	ibuf->offset = offset;
}

size_t gles20_index_buffer_get_offset(IndexBuffer *ibuf) {
	return ibuf->offset;
}

void gles20_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data) {
	gles20_ibo_index_t *indices = data;
	attr_unused size_t num_indices = data_size / sizeof(gles20_ibo_index_t);
	assert(ibuf->offset + num_indices - 1 < ibuf->num_elements);
	memcpy(ibuf->elements + ibuf->offset, indices, data_size);
	ibuf->offset += num_indices;
}

void gles20_index_buffer_destroy(IndexBuffer *ibuf) {
	mem_free(ibuf);
}

void gles20_index_buffer_flush(IndexBuffer *ibuf) {
}

void gles20_index_buffer_invalidate(IndexBuffer *ibuf) {
	ibuf->offset = 0;
}
