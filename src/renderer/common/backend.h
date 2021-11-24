/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

typedef struct FramebufferAttachmentQueryResult {
	Texture *texture;
	uint miplevel;
} FramebufferAttachmentQueryResult;

typedef struct RendererFuncs {
	void (*init)(void);
	void (*post_init)(void);
	void (*shutdown)(void);

	SDL_Window* (*create_window)(const char *title, int x, int y, int w, int h, uint32_t flags);

	r_feature_bits_t (*features)(void);

	void (*capabilities)(r_capability_bits_t capbits);
	r_capability_bits_t (*capabilities_current)(void);

	void (*draw)(VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance);
	void (*draw_indexed)(VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance);

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
	bool (*shader_object_transfer)(ShaderObject *dst, ShaderObject *src);

	ShaderProgram* (*shader_program_link)(uint num_objects, ShaderObject *shobjs[num_objects]);
	void (*shader_program_destroy)(ShaderProgram *prog);
	void (*shader_program_set_debug_label)(ShaderProgram *prog, const char *label);
	const char* (*shader_program_get_debug_label)(ShaderProgram *prog);
	bool (*shader_program_transfer)(ShaderProgram *dst, ShaderProgram *src);

	void (*shader)(ShaderProgram *prog);
	ShaderProgram* (*shader_current)(void);

	Uniform* (*shader_uniform)(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash);
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
	void (*texture_fill)(Texture *tex, uint mipmap, uint layer, const Pixmap *image_data);
	void (*texture_fill_region)(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image_data);
	bool (*texture_dump)(Texture *tex, uint mipmap, uint layer, Pixmap *dst);
	void (*texture_clear)(Texture *tex, const Color *clr);
	bool (*texture_type_query)(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result);
	bool (*texture_transfer)(Texture *dst, Texture *src);

	Framebuffer* (*framebuffer_create)(void);
	const char* (*framebuffer_get_debug_label)(Framebuffer *framebuffer);
	void (*framebuffer_set_debug_label)(Framebuffer *framebuffer, const char *label);
	void (*framebuffer_destroy)(Framebuffer *framebuffer);
	void (*framebuffer_attach)(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
	void (*framebuffer_viewport)(Framebuffer *framebuffer, FloatRect vp);
	void (*framebuffer_viewport_current)(Framebuffer *framebuffer, FloatRect *vp);
	FramebufferAttachmentQueryResult (*framebuffer_query_attachment)(Framebuffer *framebuffer, FramebufferAttachment attachment);
	void (*framebuffer_outputs)(Framebuffer *framebuffer, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS], uint8_t write_mask);
	void (*framebuffer_clear)(Framebuffer *framebuffer, ClearBufferFlags flags, const Color *colorval, float depthval);
	IntExtent (*framebuffer_get_size)(Framebuffer *framebuffer);

	void (*framebuffer)(Framebuffer *framebuffer);
	Framebuffer* (*framebuffer_current)(void);

	VertexBuffer* (*vertex_buffer_create)(size_t capacity, void *data);
	const char* (*vertex_buffer_get_debug_label)(VertexBuffer *vbuf);
	void (*vertex_buffer_set_debug_label)(VertexBuffer *vbuf, const char *label);
	void (*vertex_buffer_destroy)(VertexBuffer *vbuf);
	void (*vertex_buffer_invalidate)(VertexBuffer *vbuf);
	SDL_RWops* (*vertex_buffer_get_stream)(VertexBuffer *vbuf);

	IndexBuffer* (*index_buffer_create)(size_t max_elements);
	size_t (*index_buffer_get_capacity)(IndexBuffer *ibuf);
	const char* (*index_buffer_get_debug_label)(IndexBuffer *ibuf);
	void (*index_buffer_set_debug_label)(IndexBuffer *ibuf, const char *label);
	void (*index_buffer_set_offset)(IndexBuffer *ibuf, size_t offset);
	size_t (*index_buffer_get_offset)(IndexBuffer *ibuf);
	void (*index_buffer_add_indices)(IndexBuffer *ibuf, uint index_ofs, size_t num_indices, uint indices[num_indices]);
	void (*index_buffer_destroy)(IndexBuffer *ibuf);

	VertexArray* (*vertex_array_create)(void);
	const char* (*vertex_array_get_debug_label)(VertexArray *varr);
	void (*vertex_array_set_debug_label)(VertexArray *varr, const char *label);
	void (*vertex_array_destroy)(VertexArray *varr);
	void (*vertex_array_layout)(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]);
	void (*vertex_array_attach_vertex_buffer)(VertexArray *varr, VertexBuffer *vbuf, uint attachment);
	void (*vertex_array_attach_index_buffer)(VertexArray *varr, IndexBuffer *ibuf);
	VertexBuffer* (*vertex_array_get_vertex_attachment)(VertexArray *varr, uint attachment);
	IndexBuffer* (*vertex_array_get_index_attachment)(VertexArray *varr);

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
void _r_backend_inherit(RendererBackend *dest, const RendererBackend *base);
