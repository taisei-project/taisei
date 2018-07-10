/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "resource/resource.h"

typedef struct RendererFuncs {
	void (*init)(void);
	void (*post_init)(void);
	void (*shutdown)(void);

	SDL_Window* (*create_window)(const char *title, int x, int y, int w, int h, uint32_t flags);

	bool (*supports)(RendererFeature feature);

	void (*capabilities)(r_capability_bits_t capbits);
	r_capability_bits_t (*capabilities_current)(void);

	void (*draw)(Primitive prim, uint first, uint count, uint32_t * indices, uint instances, uint base_instance);

	void (*color4)(float r, float g, float b, float a);
	Color (*color_current)(void);

	void (*blend)(BlendMode mode);
	BlendMode (*blend_current)(void);

	void (*cull)(CullFaceMode mode);
	CullFaceMode (*cull_current)(void);

	void (*depth_func)(DepthTestFunc depth_func);
	DepthTestFunc (*depth_func_current)(void);

	void (*shader)(ShaderProgram *prog);
	ShaderProgram* (*shader_current)(void);

	Uniform* (*shader_uniform)(ShaderProgram *prog, const char *uniform_name);
	void (*uniform)(Uniform *uniform, uint count, const void *data);
	UniformType (*uniform_type)(Uniform *uniform);

	void (*texture_create)(Texture *tex, const TextureParams *params);
	void (*texture_get_params)(Texture *tex, TextureParams *params);
	void (*texture_set_filter)(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag);
	void (*texture_set_wrap)(Texture *tex, TextureWrapMode ws, TextureWrapMode wt);
	void (*texture_destroy)(Texture *tex);
	void (*texture_invalidate)(Texture *tex);
	void (*texture_fill)(Texture *tex, uint mipmap, void *image_data);
	void (*texture_fill_region)(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data);

	void (*texture)(uint unit, Texture *tex);
	Texture* (*texture_current)(uint unit);

	void (*framebuffer_create)(Framebuffer *framebuffer);
	void (*framebuffer_destroy)(Framebuffer *framebuffer);
	void (*framebuffer_attach)(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
	void (*framebuffer_viewport)(Framebuffer *framebuffer, IntRect vp);
	void (*framebuffer_viewport_current)(Framebuffer *framebuffer, IntRect *vp);
	Texture* (*framebuffer_get_attachment)(Framebuffer *framebuffer, FramebufferAttachment attachment);
	uint (*framebuffer_get_attachment_mipmap)(Framebuffer *framebuffer, FramebufferAttachment attachment);

	void (*framebuffer)(Framebuffer *framebuffer);
	Framebuffer* (*framebuffer_current)(void);

	void (*vertex_buffer_create)(VertexBuffer *vbuf, size_t capacity, void *data);
	void (*vertex_buffer_destroy)(VertexBuffer *vbuf);
	void (*vertex_buffer_invalidate)(VertexBuffer *vbuf);
	void (*vertex_buffer_write)(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data);
	void (*vertex_buffer_append)(VertexBuffer *vbuf, size_t data_size, void *data);

	void (*vertex_array_create)(VertexArray *varr);
	void (*vertex_array_destroy)(VertexArray *varr);
	void (*vertex_array_layout)(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
	void (*vertex_array_attach_buffer)(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
	VertexBuffer* (*vertex_array_get_attachment)(VertexArray *varr, uint attachment);

	void (*vertex_array)(VertexArray *varr) attr_nonnull(1);
	VertexArray* (*vertex_array_current)(void);

	void (*clear)(ClearBufferFlags flags);
	void (*clear_color4)(float r, float g, float b, float a);
	Color (*clear_color_current)(void);

	void (*vsync)(VsyncMode mode);
	VsyncMode (*vsync_current)(void);

	void (*swap)(SDL_Window *window);

	uint8_t* (*screenshot)(uint *out_width, uint *out_height);
} RendererFuncs;

typedef struct RendererBackend {
	const char *name;
	RendererFuncs funcs;

	struct {
		ResourceHandler *shader_object;
		ResourceHandler *shader_program;
	} res_handlers;

	void *custom;
} RendererBackend;

extern RendererBackend *_r_backends[];
extern RendererBackend _r_backend;

void _r_backend_init(void);
