/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "models.h"
#include "../api.h"
#include "resource/model.h"

static struct {
	VertexBuffer *vbuf;
	IndexBuffer *ibuf;
	VertexArray *varr;
	Model quad;
} _r_models;

void _r_models_init(void) {
	VertexAttribSpec spec[] = {
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // position
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT }, // texcoord
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // normal
		{ 4, VA_FLOAT, VA_CONVERT_FLOAT }, // tangent
	};

	VertexAttribFormat fmt[ARRAY_SIZE(spec)];

	r_vertex_attrib_format_interleaved(ARRAY_SIZE(spec), spec, fmt, 0);

	GenericModelVertex quad[4] = {
		{ {  0.5, -0.5,  0.0 }, { 1, 1 }, { 0, 0, 1 }, { 0, 0, 0, 0 } },
		{ {  0.5,  0.5,  0.0 }, { 1, 0 }, { 0, 0, 1 }, { 0, 0, 0, 0 } },
		{ { -0.5, -0.5,  0.0 }, { 0, 1 }, { 0, 0, 1 }, { 0, 0, 0, 0 } },
		{ { -0.5,  0.5,  0.0 }, { 0, 0 }, { 0, 0, 1 }, { 0, 0, 0, 0 } },
	};

	const size_t max_vertices = 100000;

	_r_models.vbuf = r_vertex_buffer_create(max_vertices * sizeof(GenericModelVertex), NULL);
	r_vertex_buffer_set_debug_label(_r_models.vbuf, "Static models vertex buffer");

	_r_models.ibuf = r_index_buffer_create(max_vertices);
	r_index_buffer_set_debug_label(_r_models.ibuf, "Static models index buffer");

	_r_models.varr = r_vertex_array_create();
	r_vertex_array_set_debug_label(_r_models.varr, "Static models vertex array");
	r_vertex_array_layout(_r_models.varr, ARRAY_SIZE(fmt), fmt);
	r_vertex_array_attach_vertex_buffer(_r_models.varr, _r_models.vbuf, 0);
	r_vertex_array_attach_index_buffer(_r_models.varr, _r_models.ibuf);

	r_model_add_static(&_r_models.quad, PRIM_TRIANGLE_STRIP, 4, quad, 0, NULL);
}

void _r_models_shutdown(void) {
	r_vertex_array_destroy(_r_models.varr);
	r_vertex_buffer_destroy(_r_models.vbuf);
}

void r_model_add_static(
	Model *out_mdl,
	Primitive prim,
	size_t num_vertices,
	GenericModelVertex vertices[num_vertices],
	size_t num_indices,
	uint indices[num_indices]
) {
	out_mdl->vertex_array = _r_models.varr;
	out_mdl->num_vertices = num_vertices;
	out_mdl->num_indices = num_indices;
	out_mdl->primitive = prim;

	SDL_RWops *vert_stream = r_vertex_buffer_get_stream(_r_models.vbuf);
	size_t vert_ofs = SDL_RWtell(vert_stream) / sizeof(GenericModelVertex);

	if(num_indices > 0) {
		assume(indices != NULL);
		out_mdl->offset = r_index_buffer_get_offset(_r_models.ibuf);
		r_index_buffer_add_indices(_r_models.ibuf, vert_ofs, num_indices, indices);
	} else {
		out_mdl->offset = vert_ofs;
	}

	SDL_RWwrite(vert_stream, vertices, sizeof(GenericModelVertex), num_vertices);
}

VertexBuffer* r_vertex_buffer_static_models(void) {
	return _r_models.vbuf;
}

VertexArray* r_vertex_array_static_models(void) {
	return _r_models.varr;
}

void r_draw_quad(void) {
	r_draw_model_ptr(&_r_models.quad, 0, 0);
}

void r_draw_quad_instanced(uint instances) {
	r_draw_model_ptr(&_r_models.quad, instances, 0);
}

void r_draw_model_ptr(Model *model, uint instances, uint base_instance) {
	if(model->num_indices) {
		r_draw_indexed(
			model->vertex_array,
			model->primitive,
			model->offset, model->num_indices,
			instances,
			base_instance
		);
	} else {
		r_draw(
			model->vertex_array,
			model->primitive,
			model->offset, model->num_vertices,
			instances,
			base_instance
		);
	}
}
