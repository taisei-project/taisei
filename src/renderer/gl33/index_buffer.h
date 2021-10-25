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
} IndexBuffer;

// Unfortunately, we can't use short indices unless we use glDrawElementsBaseVertex,
// which is not available in unextended GLES3/WebGL.

#if 0
typedef GLushort gl33_ibo_index_t;
#define GL33_IBO_MAX_INDEX UINT16_MAX
#define GL33_IBO_GL_DATATYPE GL_UNSIGNED_SHORT
#else
typedef GLuint gl33_ibo_index_t;
#define GL33_IBO_MAX_INDEX UINT32_MAX
#define GL33_IBO_GL_DATATYPE GL_UNSIGNED_INT
#endif

IndexBuffer *gl33_index_buffer_create(size_t max_elements);
size_t gl33_index_buffer_get_capacity(IndexBuffer *ibuf);
const char *gl33_index_buffer_get_debug_label(IndexBuffer *ibuf);
void gl33_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label);
void gl33_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset);
size_t gl33_index_buffer_get_offset(IndexBuffer *ibuf);
void gl33_index_buffer_add_indices(IndexBuffer *ibuf, uint index_ofs, size_t num_indices, uint indices[num_indices]);
void gl33_index_buffer_destroy(IndexBuffer *ibuf);
void gl33_index_buffer_flush(IndexBuffer *ibuf);
void gl33_index_buffer_on_vao_attach(IndexBuffer *ibuf, GLuint vao);
