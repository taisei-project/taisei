/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "vertex_buffer.h"

VertexBuffer *sdlgpu_vertex_buffer_create(size_t capacity, void *data) {
	auto vbuf = (VertexBuffer*)sdlgpu_buffer_create(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(VertexBuffer));
	cachedbuf_resize(&vbuf->cbuf.cachedbuf, capacity);

	if(data) {
		memcpy(vbuf->cbuf.cachedbuf.cache, data, capacity);
	}

	return vbuf;
}

const char *sdlgpu_vertex_buffer_get_debug_label(VertexBuffer *vbuf) {
	return vbuf->cbuf.debug_label;
}

void sdlgpu_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label) {
	sdlgpu_buffer_set_debug_label(&vbuf->cbuf, label);
}

void sdlgpu_vertex_buffer_destroy(VertexBuffer *vbuf) {
	sdlgpu_buffer_destroy(&vbuf->cbuf);
}

void sdlgpu_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	sdlgpu_buffer_invalidate(&vbuf->cbuf);
}

SDL_IOStream *sdlgpu_vertex_buffer_get_stream(VertexBuffer *vbuf) {
	return sdlgpu_buffer_get_stream(&vbuf->cbuf);
}

void sdlgpu_vertex_buffer_flush(VertexBuffer *vbuf) {
	sdlgpu_buffer_flush(&vbuf->cbuf);
}
