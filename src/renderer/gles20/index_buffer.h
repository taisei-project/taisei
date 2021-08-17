/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../glcommon/opengl.h"
#include "../api.h"

typedef GLushort gles20_ibo_index_t;
#define GLES20_IBO_MAX_INDEX UINT16_MAX
#define GLES20_IBO_GL_DATATYPE GL_UNSIGNED_SHORT

typedef struct IndexBuffer {
	size_t num_elements;
	size_t offset;
	char debug_label[R_DEBUG_LABEL_SIZE];
	gles20_ibo_index_t elements[];
} IndexBuffer;

IndexBuffer *gles20_index_buffer_create(uint index_size, size_t max_elements);
size_t gles20_index_buffer_get_capacity(IndexBuffer *ibuf);
uint gles20_index_buffer_get_index_size(IndexBuffer *ibuf);
const char *gles20_index_buffer_get_debug_label(IndexBuffer *ibuf);
void gles20_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label);
void gles20_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset);
size_t gles20_index_buffer_get_offset(IndexBuffer *ibuf);
void gles20_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data);
void gles20_index_buffer_destroy(IndexBuffer *ibuf);
void gles20_index_buffer_flush(IndexBuffer *ibuf);
void gles20_index_buffer_invalidate(IndexBuffer *ibuf);
