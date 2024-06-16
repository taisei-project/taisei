/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "common_buffer.h"

typedef struct IndexBuffer {
	CommonBuffer cbuf;
	uint index_size;
} IndexBuffer;

IndexBuffer *sdlgpu_index_buffer_create(uint index_size, size_t max_elements);
size_t sdlgpu_index_buffer_get_capacity(IndexBuffer *ibuf);
uint sdlgpu_index_buffer_get_index_size(IndexBuffer *ibuf);
const char *sdlgpu_index_buffer_get_debug_label(IndexBuffer *ibuf);
void sdlgpu_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label);
void sdlgpu_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset);
size_t sdlgpu_index_buffer_get_offset(IndexBuffer *ibuf);
void sdlgpu_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data);
void sdlgpu_index_buffer_destroy(IndexBuffer *ibuf);
void sdlgpu_index_buffer_flush(IndexBuffer *ibuf);
void sdlgpu_index_buffer_invalidate(IndexBuffer *ibuf);
