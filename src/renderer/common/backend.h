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
	const Color* (*color_current)(void);

	void (*blend)(BlendMode mode);
	BlendMode (*blend_current)(void);

	void (*cull)(CullFaceMode mode);
	CullFaceMode (*cull_current)(void);

	void (*depth_func)(DepthTestFunc depth_func);
	DepthTestFunc (*depth_func_current)(void);

	bool (*shader_language_supported)(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative);

	ShaderObject* (*shader_object_compile)(ShaderSource *source);
	void (*shader_object_destroy)(ShaderObject *shobj);
	void (*shader_object_set_debug_label)(ShaderObject *shobj, const char *label);
	const char* (*shader_object_get_debug_label)(ShaderObject *shobj);

	ShaderProgram* (*shader_program_link)(uint num_objects, ShaderObject *shobjs[num_objects]);
	void (*shader_program_destroy)(ShaderProgram *prog);
	void (*shader_program_set_debug_label)(ShaderProgram *prog, const char *label);
	const char* (*shader_program_get_debug_label)(ShaderProgram *prog);

	void (*shader)(ShaderProgram *prog);
	ShaderProgram* (*shader_current)(void);

	Uniform* (*shader_uniform)(ShaderProgram *prog, const char *uniform_name);
	void (*uniform)(Uniform *uniform, uint offset, uint count, const void *data);
	UniformType (*uniform_type)(Uniform *uniform);

	Texture* (*texture_create)(const TextureParams *params);
	void (*texture_get_params)(Texture *tex, TextureParams *params);
	void (*texture_get_size)(Texture *tex, uint mipmap, uint *width, uint *height);
	const char* (*texture_get_debug_label)(Texture *tex);
	void (*texture_set_debug_label)(Texture *tex, const char *label);
	void (*texture_set_filter)(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag);
	void (*texture_set_wrap)(Texture *tex, TextureWrapMode ws, TextureWrapMode wt);
	void (*texture_destroy)(Texture *tex);
	void (*texture_invalidate)(Texture *tex);
	void (*texture_fill)(Texture *tex, uint mipmap, const Pixmap *image_data);
	void (*texture_fill_region)(Texture *tex, uint mipmap, uint x, uint y, const Pixmap *image_data);
	void (*texture_clear)(Texture *tex, const Color *clr);

	Framebuffer* (*framebuffer_create)(void);
	const char* (*framebuffer_get_debug_label)(Framebuffer *framebuffer);
	void (*framebuffer_set_debug_label)(Framebuffer *framebuffer, const char *label);
	void (*framebuffer_destroy)(Framebuffer *framebuffer);
	void (*framebuffer_attach)(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
	void (*framebuffer_viewport)(Framebuffer *framebuffer, IntRect vp);
	void (*framebuffer_viewport_current)(Framebuffer *framebuffer, IntRect *vp);
	Texture* (*framebuffer_get_attachment)(Framebuffer *framebuffer, FramebufferAttachment attachment);
	uint (*framebuffer_get_attachment_mipmap)(Framebuffer *framebuffer, FramebufferAttachment attachment);
	void (*framebuffer_clear)(Framebuffer *framebuffer, ClearBufferFlags flags, const Color *colorval, float depthval);

	void (*framebuffer)(Framebuffer *framebuffer);
	Framebuffer* (*framebuffer_current)(void);

	VertexBuffer* (*vertex_buffer_create)(size_t capacity, void *data);
	const char* (*vertex_buffer_get_debug_label)(VertexBuffer *vbuf);
	void (*vertex_buffer_set_debug_label)(VertexBuffer *vbuf, const char *label);
	void (*vertex_buffer_destroy)(VertexBuffer *vbuf);
	void (*vertex_buffer_invalidate)(VertexBuffer *vbuf);
	SDL_RWops* (*vertex_buffer_get_stream)(VertexBuffer *vbuf);

	VertexArray* (*vertex_array_create)(void);
	const char* (*vertex_array_get_debug_label)(VertexArray *varr);
	void (*vertex_array_set_debug_label)(VertexArray *varr, const char *label);
	void (*vertex_array_destroy)(VertexArray *varr);
	void (*vertex_array_layout)(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
	void (*vertex_array_attach_buffer)(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
	VertexBuffer* (*vertex_array_get_attachment)(VertexArray *varr, uint attachment);

	void (*vertex_array)(VertexArray *varr) attr_nonnull(1);
	VertexArray* (*vertex_array_current)(void);

	void (*vsync)(VsyncMode mode);
	VsyncMode (*vsync_current)(void);

	void (*swap)(SDL_Window *window);

	bool (*screenshot)(Pixmap *dst);
} RendererFuncs;

typedef struct RendererBackend {
	const char *name;
	RendererFuncs funcs;
	void *custom;
} RendererBackend;

extern RendererBackend *_r_backends[];
extern RendererBackend _r_backend;

void _r_backend_init(void);
