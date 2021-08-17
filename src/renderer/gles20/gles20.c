/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "gles20.h"
#include "../glescommon/gles.h"
#include "../gl33/gl33.h"
#include "../gl33/vertex_array.h"
#include "index_buffer.h"

static void gles20_init(void) {
	gles_init(&_r_backend_gles20, 2, 0);
}

static void gles20_init_context(SDL_Window *w) {
	gles_init_context(w);

	if(!glext.vertex_array_object) {
		log_fatal("GL doesn't support VAOs; no fallback implemented yet, sorry.");
	}
}

static void gles20_draw_indexed(VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance) {
	assert(count > 0);
	assert(base_instance == 0);
	assert(varr->index_attachment != NULL);
	GLuint gl_prim = gl33_prim_to_gl_prim(prim);

	void *state;
	gl33_begin_draw(varr, &state);

	IndexBuffer *ibuf = varr->index_attachment;
	gles20_ibo_index_t *indices = ibuf->elements + firstidx;
	assert(indices < ibuf->elements + ibuf->num_elements);

	if(instances) {
		glDrawElementsInstanced(gl_prim, count, GLES20_IBO_GL_DATATYPE, indices, instances);
	} else {
		glDrawElements(gl_prim, count, GLES20_IBO_GL_DATATYPE, indices);
	}

	gl33_end_draw(state);
}

static void gles20_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf) {
	// We override this function to prevent setting the index buffer dirty bit.
	// Otherwise the GL33 VAO code would try to dereference index_attachment as
	// another kind of struct (the GL33 IBO implementation).
	varr->index_attachment = ibuf;
}

RendererBackend _r_backend_gles20 = {
	.name = "gles20",
	.funcs = {
		.init = gles20_init,
		.texture_dump = gles_texture_dump,
		.screenshot = gles_screenshot,
		.index_buffer_create = gles20_index_buffer_create,
		.index_buffer_get_capacity = gles20_index_buffer_get_capacity,
		.index_buffer_get_index_size = gles20_index_buffer_get_index_size,
		.index_buffer_get_debug_label = gles20_index_buffer_get_debug_label,
		.index_buffer_set_debug_label = gles20_index_buffer_set_debug_label,
		.index_buffer_set_offset = gles20_index_buffer_set_offset,
		.index_buffer_get_offset = gles20_index_buffer_get_offset,
		.index_buffer_add_indices = gles20_index_buffer_add_indices,
		.index_buffer_invalidate = gles20_index_buffer_invalidate,
		.index_buffer_destroy = gles20_index_buffer_destroy,
		.draw_indexed = gles20_draw_indexed,
		.vertex_array_attach_index_buffer = gles20_vertex_array_attach_index_buffer,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.init_context = gles20_init_context,
		}
	},
};
