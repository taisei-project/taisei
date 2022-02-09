/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "common_buffer.h"

typedef struct IndexBuffer {
	CommonBuffer cbuf;
	GLuint vao;
	GLuint prev_vao;
	uint index_size;
	GLenum index_datatype;
} IndexBuffer;

IndexBuffer *gl33_index_buffer_create(uint index_size, size_t max_elements);
size_t gl33_index_buffer_get_capacity(IndexBuffer *ibuf);
uint gl33_index_buffer_get_index_size(IndexBuffer *ibuf);
const char *gl33_index_buffer_get_debug_label(IndexBuffer *ibuf);
void gl33_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label);
void gl33_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset);
size_t gl33_index_buffer_get_offset(IndexBuffer *ibuf);
void gl33_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data);
void gl33_index_buffer_destroy(IndexBuffer *ibuf);
void gl33_index_buffer_flush(IndexBuffer *ibuf);
void gl33_index_buffer_on_vao_attach(IndexBuffer *ibuf, GLuint vao);
