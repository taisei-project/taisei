/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "sdlgpu.h"

#include "../api.h"

struct VertexArray {
	DYNAMIC_ARRAY(VertexBuffer*) attachments;
	IndexBuffer *index_attachment;

	SDL_GPUVertexInputState vertex_input_state;
	uint *binding_to_attachment_map;

	sdlgpu_id_t layout_id;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

VertexArray *sdlgpu_vertex_array_create(void);
const char *sdlgpu_vertex_array_get_debug_label(VertexArray *varr);
void sdlgpu_vertex_array_set_debug_label(VertexArray *varr, const char *label);
void sdlgpu_vertex_array_destroy(VertexArray *varr);
void sdlgpu_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
void sdlgpu_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf);
VertexBuffer *sdlgpu_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment);
IndexBuffer *sdlgpu_vertex_array_get_index_attachment(VertexArray *varr);
void sdlgpu_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
void sdlgpu_vertex_array_flush_buffers(VertexArray *varr);
