/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "models.h"
#include "../api.h"

static struct {
	VertexBuffer vbuf;
	VertexArray varr;
} _r_models;

void _r_models_init(void) {
	VertexAttribFormat fmt[3];

	r_vertex_attrib_format_interleaved(3, (VertexAttribSpec[]) {
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // position
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // normal
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT }, // texcoord
	}, fmt, 0);

	GenericModelVertex quad[4] = {
		{ { -0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 0, 0 } },
		{ { -0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 0, 1 } },
		{ {  0.5,  0.5,  0.0 }, { 0, 0, 1 }, { 1, 1 } },
		{ {  0.5, -0.5,  0.0 }, { 0, 0, 1 }, { 1, 0 } },
	};

	r_vertex_buffer_create(&_r_models.vbuf, 8192 * sizeof(GenericModelVertex), NULL);

	r_vertex_array_create(&_r_models.varr);
	r_vertex_array_layout(&_r_models.varr, 3, fmt);
	r_vertex_array_attach_buffer(&_r_models.varr, &_r_models.vbuf, 0);

	r_vertex_buffer_append(&_r_models.vbuf, sizeof(quad), quad);
	r_vertex_array(&_r_models.varr);
}

void _r_models_shutdown(void) {
	r_vertex_array_destroy(&_r_models.varr);
	r_vertex_buffer_destroy(&_r_models.vbuf);
}

VertexBuffer* r_vertex_buffer_static_models(void) {
	return &_r_models.vbuf;
}

VertexArray* r_vertex_array_static_models(void) {
	return &_r_models.varr;
}

void r_draw_quad(void) {
	VertexArray *varr_saved = r_vertex_array_current();
	r_vertex_array(&_r_models.varr);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, 0);
	r_vertex_array(varr_saved);
}

void r_draw_quad_instanced(uint instances) {
	VertexArray *varr_saved = r_vertex_array_current();
	r_vertex_array(&_r_models.varr);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, instances);
	r_vertex_array(varr_saved);
}

void r_draw_model_ptr(Model *model) {
	VertexArray *varr_saved = r_vertex_array_current();
	r_vertex_array(&_r_models.varr);
	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_scale(1, -1, 1); // XXX: flipped texture workaround. can we get rid of this somehow?
	r_draw(PRIM_TRIANGLES, 0, model->icount, model->indices, 0);
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_vertex_array(varr_saved);
}
