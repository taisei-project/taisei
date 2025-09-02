/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "index_buffer.h"

IndexBuffer *sdlgpu_index_buffer_create(uint index_size, size_t max_elements) {
	assert(index_size == 2 || index_size == 4);

	size_t capacity = max_elements * index_size;
	auto ibuf = (IndexBuffer*)sdlgpu_buffer_create(SDL_GPU_BUFFERUSAGE_INDEX, sizeof(IndexBuffer));
	cachedbuf_resize(&ibuf->cbuf.cachedbuf, capacity);
	ibuf->index_size = index_size;

	return ibuf;
}

size_t sdlgpu_index_buffer_get_capacity(IndexBuffer *ibuf) {
	return ibuf->cbuf.cachedbuf.size / ibuf->index_size;
}

uint sdlgpu_index_buffer_get_index_size(IndexBuffer *ibuf) {
	return ibuf->index_size;
}

const char *sdlgpu_index_buffer_get_debug_label(IndexBuffer *ibuf) {
	return ibuf->cbuf.debug_label;
}

void sdlgpu_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) {
	sdlgpu_buffer_set_debug_label(&ibuf->cbuf, label);
}

void sdlgpu_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) {
	ibuf->cbuf.cachedbuf.stream_offset = offset * ibuf->index_size;
}

size_t sdlgpu_index_buffer_get_offset(IndexBuffer *ibuf) {
	return ibuf->cbuf.cachedbuf.stream_offset / ibuf->index_size;
}

void sdlgpu_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data) {
	SDL_IOStream *stream = sdlgpu_buffer_get_stream(&ibuf->cbuf);
	SDL_WriteIO(stream, data, data_size);
}

void sdlgpu_index_buffer_destroy(IndexBuffer *ibuf) {
	sdlgpu_buffer_destroy(&ibuf->cbuf);
}

void sdlgpu_index_buffer_flush(IndexBuffer *ibuf) {
	sdlgpu_buffer_flush(&ibuf->cbuf);
}

void sdlgpu_index_buffer_invalidate(IndexBuffer *ibuf) {
	sdlgpu_buffer_invalidate(&ibuf->cbuf);
}
