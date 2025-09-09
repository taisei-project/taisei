/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "vertex_array.h"

#include "index_buffer.h"
#include "pipeline_cache.h"
#include "vertex_buffer.h"

#include "util.h"

static SDL_GPUVertexElementFormat vertex_elem_format(VertexAttribType type, VertexAttribConversion conv, uint vsize) {
	static struct {
		uint8_t elements;
		uint8_t type;
		uint8_t conversion;
	} formats[] = {
		[SDL_GPU_VERTEXELEMENTFORMAT_INT]              = { 1, VA_INT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_INT2]             = { 2, VA_INT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_INT3]             = { 3, VA_INT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_INT4]             = { 4, VA_INT, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_UINT]             = { 1, VA_UINT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_UINT2]            = { 2, VA_UINT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_UINT3]            = { 3, VA_UINT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_UINT4]            = { 4, VA_UINT, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_FLOAT]            = { 1, VA_FLOAT, VA_CONVERT_FLOAT },
		[SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2]           = { 2, VA_FLOAT, VA_CONVERT_FLOAT },
		[SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3]           = { 3, VA_FLOAT, VA_CONVERT_FLOAT },
		[SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4]           = { 4, VA_FLOAT, VA_CONVERT_FLOAT },

		[SDL_GPU_VERTEXELEMENTFORMAT_BYTE2]            = { 2, VA_BYTE, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_BYTE4]            = { 4, VA_BYTE, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2]           = { 2, VA_UBYTE, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4]           = { 4, VA_UBYTE, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_BYTE2_NORM]       = { 2, VA_BYTE, VA_CONVERT_FLOAT_NORMALIZED },
		[SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM]       = { 4, VA_BYTE, VA_CONVERT_FLOAT_NORMALIZED },

		[SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2_NORM]      = { 2, VA_UBYTE, VA_CONVERT_FLOAT_NORMALIZED },
		[SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM]      = { 4, VA_UBYTE, VA_CONVERT_FLOAT_NORMALIZED },

		[SDL_GPU_VERTEXELEMENTFORMAT_SHORT2]           = { 2, VA_SHORT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_SHORT4]           = { 4, VA_SHORT, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_USHORT2]          = { 2, VA_USHORT, VA_CONVERT_INT },
		[SDL_GPU_VERTEXELEMENTFORMAT_USHORT4]          = { 4, VA_USHORT, VA_CONVERT_INT },

		[SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM]      = { 2, VA_SHORT, VA_CONVERT_FLOAT_NORMALIZED },
		[SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM]      = { 4, VA_SHORT, VA_CONVERT_FLOAT_NORMALIZED },

		[SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM]     = { 2, VA_USHORT, VA_CONVERT_FLOAT_NORMALIZED },
		[SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM]     = { 4, VA_USHORT, VA_CONVERT_FLOAT_NORMALIZED },

		// Not supported by our API yet
		// [SDL_GPU_VERTEXELEMENTFORMAT_HALF2]         = {},
		// [SDL_GPU_VERTEXELEMENTFORMAT_HALF4]         = {},
	};

	for(SDL_GPUVertexElementFormat fmt = 0; fmt < ARRAY_SIZE(formats); ++fmt) {
		if(
			formats[fmt].type == type &&
			formats[fmt].conversion == conv &&
			formats[fmt].elements == vsize
		) {
			return fmt;
		}
	}

	log_fatal("Vertex attribute format not supported: %u %u %u", type, conv, vsize);
}

VertexArray *sdlgpu_vertex_array_create(void) {
	auto varr = ALLOC(VertexArray);
	return varr;
}

const char *sdlgpu_vertex_array_get_debug_label(VertexArray *varr) {
	return varr->debug_label;
}

void sdlgpu_vertex_array_set_debug_label(VertexArray *varr, const char *label) {
	strlcpy(varr->debug_label, label, sizeof(varr->debug_label));
}

void sdlgpu_vertex_array_destroy(VertexArray *varr) {
	sdlgpu_pipecache_unref_vertex_array(varr->layout_id);
	dynarray_free_data(&varr->attachments);
	mem_free((void*)varr->vertex_input_state.vertex_attributes);
	mem_free((void*)varr->vertex_input_state.vertex_buffer_descriptions);
	mem_free(varr->binding_to_attachment_map);
	mem_free(varr);
}

void sdlgpu_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) {
	dynarray_ensure_capacity(&varr->attachments, attachment + 1);
	varr->attachments.num_elements = max(attachment + 1, varr->attachments.num_elements);
	dynarray_set(&varr->attachments, attachment, vbuf);
}

void sdlgpu_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *ibuf) {
	varr->index_attachment = ibuf;
}

VertexBuffer *sdlgpu_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment) {
	return dynarray_get(&varr->attachments, attachment);
}

IndexBuffer *sdlgpu_vertex_array_get_index_attachment(VertexArray *varr) {
	return varr->index_attachment;
}

void sdlgpu_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) {
	SDL_GPUVertexAttribute sdl_attribs[nattribs];
	SDL_GPUVertexBufferDescription sdl_bindings[nattribs];
	uint num_sdl_bindings = 0;

	for(uint i = 0; i < nattribs; ++i) {
		auto want_binding = (SDL_GPUVertexBufferDescription) {
			// Temporarily store taisei-side buffer attachment index here.
			// We need this to figure out which bindings are identical.
			// Later we will create a map of SDLGPU buffer indices to taisei ones and rewrite this.
			.slot = attribs[i].attachment,
			.input_rate = attribs[i].spec.divisor ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX,
			.instance_step_rate = 0,
			.pitch = attribs[i].stride,
		};

		uint binding_idx = num_sdl_bindings;

		for(uint j = 0; j < num_sdl_bindings; ++j) {
			if(!memcmp(&sdl_bindings[j], &want_binding, sizeof(want_binding))) {
				binding_idx = j;
				break;
			}
		}

		if(binding_idx == num_sdl_bindings) {
			++num_sdl_bindings;
			sdl_bindings[binding_idx] = want_binding;
		}

		sdl_attribs[i] = (SDL_GPUVertexAttribute) {
			.buffer_slot = binding_idx,
			.format = vertex_elem_format(attribs[i].spec.type, attribs[i].spec.conversion, attribs[i].spec.elements),
			.location = i,
			.offset = attribs[i].offset,
		};
	}

	mem_free(varr->binding_to_attachment_map);
	varr->binding_to_attachment_map = mem_realloc(
		varr->binding_to_attachment_map, num_sdl_bindings * sizeof(*varr->binding_to_attachment_map));

	for(uint i = 0; i < num_sdl_bindings; ++i) {
		varr->binding_to_attachment_map[i] = sdl_bindings[i].slot;
		sdl_bindings[i].slot = i;
	}

	varr->vertex_input_state.vertex_attributes = mem_realloc(
		(void*)varr->vertex_input_state.vertex_attributes, sizeof(sdl_attribs));
	memcpy((void*)varr->vertex_input_state.vertex_attributes, sdl_attribs, sizeof(sdl_attribs));
	varr->vertex_input_state.num_vertex_attributes = nattribs;

	size_t sizeof_bindings = num_sdl_bindings * sizeof(*sdl_bindings);
	varr->vertex_input_state.vertex_buffer_descriptions = mem_realloc(
		(void*)varr->vertex_input_state.vertex_buffer_descriptions, sizeof_bindings);
	memcpy((void*)varr->vertex_input_state.vertex_buffer_descriptions, sdl_bindings, sizeof_bindings);
	varr->vertex_input_state.num_vertex_buffers = num_sdl_bindings;

	if(varr->layout_id) {
		sdlgpu_pipecache_unref_vertex_array(varr->layout_id);
	}

	varr->layout_id = ++sdlgpu.ids.vertex_arrays;
}

void sdlgpu_vertex_array_flush_buffers(VertexArray *varr) {
	dynarray_foreach_elem(&varr->attachments, auto pvbuf, {
		if(*pvbuf) {
			sdlgpu_vertex_buffer_flush(*pvbuf);
		}
	});

	if(varr->index_attachment) {
		sdlgpu_index_buffer_flush(varr->index_attachment);
	}
}
