/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "../api.h"
#include "resource/shader_object.h"
#include "../common/backend.h"

static char placeholder;
static Color dummycolor;

static SDL_Window* null_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	return SDL_CreateWindow(title, x, y, w, h, flags);
}

static void null_init(void) { }
static void null_post_init(void) { }
static void null_shutdown(void) { }

static r_feature_bits_t null_features(void) { return ~0; }

static void null_capabilities(r_capability_bits_t capbits) { }
static r_capability_bits_t null_capabilities_current(void) { return (r_capability_bits_t)-1; }

static void null_color4(float r, float g, float b, float a) { }
static const Color* null_color_current(void) { return &dummycolor; }

static void null_blend(BlendMode mode) { }
static BlendMode null_blend_current(void) { return BLEND_NONE; }

static void null_cull(CullFaceMode mode) { }
static CullFaceMode null_cull_current(void) { return CULL_BACK; }

static void null_depth_func(DepthTestFunc func) { }
static DepthTestFunc null_depth_func_current(void) { return DEPTH_LESS; }

static bool null_shader_language_supported(const ShaderLangInfo *lang, ShaderLangInfo *out_alternative) { return true; }

static ShaderObject* null_shader_object_compile(ShaderSource *source) { return (void*)&placeholder; }
static void null_shader_object_destroy(ShaderObject *shobj) { }
static void null_shader_object_set_debug_label(ShaderObject *shobj, const char *label) { }
static const char* null_shader_object_get_debug_label(ShaderObject *shobj) { return "Null shader object"; }
static bool null_shader_object_transfer(ShaderObject *dst, ShaderObject *src) { return true; }

static ShaderProgram* null_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) { return (void*)&placeholder; }
static void null_shader_program_destroy(ShaderProgram *prog) { }
static void null_shader_program_set_debug_label(ShaderProgram *prog, const char *label) { }
static const char* null_shader_program_get_debug_label(ShaderProgram *prog) { return "Null shader program"; }
static bool null_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src) { return true; }

static void null_shader(ShaderProgram *prog) { }
static ShaderProgram* null_shader_current(void) { return (void*)&placeholder; }

static Uniform* null_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash) {
	return NULL;
}

static void null_uniform(Uniform *uniform, uint offset, uint count, const void *data) { }

static UniformType null_uniform_type(Uniform *uniform) { return UNIFORM_FLOAT; }

static void null_draw(VertexArray *varr, Primitive prim, uint first, uint count, uint instances, uint base_instance) { }

static Texture* null_texture_create(const TextureParams *params) {
	return (void*)&placeholder;
}

static void null_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) {
	if(width) *width = 1;
	if(height) *height = 1;
}

static void null_texture_get_params(Texture *tex, TextureParams *params) {
	memset(params, 0, sizeof(*params));
	params->width = 1;
	params->height = 1;
	params->type = TEX_TYPE_RGBA;
}

static void null_texture_set_debug_label(Texture *tex, const char *label) { }
static const char* null_texture_get_debug_label(Texture *tex) { return "null texture"; }
static void null_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) { }
static void null_texture_set_wrap(Texture *tex, TextureWrapMode fmin, TextureWrapMode fmag) { }
static void null_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image_data) { }
static void null_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image_data) { }
static bool null_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst) { return false; }
static void null_texture_invalidate(Texture *tex) { }
static void null_texture_destroy(Texture *tex) { }
static void null_texture_clear(Texture *tex, const Color *color) { }
static bool null_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result) {
	if(result) {
		result->optimal_pixmap_format = pxfmt;
		result->optimal_pixmap_origin = pxorigin;
		result->supplied_pixmap_format_supported = true;
		result->supplied_pixmap_origin_supported = true;
	}

	return true;
}
static bool null_texture_transfer(Texture *dst, Texture *src) { return true; }

static FloatRect default_fb_viewport = { 0, 0, 800, 600 };

static Framebuffer* null_framebuffer_create(void) { return (void*)&placeholder; }
static void null_framebuffer_set_debug_label(Framebuffer *fb, const char *label) { }
static const char* null_framebuffer_get_debug_label(Framebuffer *fb) { return "null framebuffer"; }
static void null_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment) { }
static FramebufferAttachmentQueryResult null_framebuffer_query_attachment(Framebuffer *fb, FramebufferAttachment attachment) {
	return (FramebufferAttachmentQueryResult) {
		.texture = (void*)&placeholder,
		.miplevel = 0,
	};
}
static void null_framebuffer_outputs(Framebuffer *fb, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS], uint8_t write_mask) {
	if(write_mask == 0x00) {
		for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
			config[i] = FRAMEBUFFER_ATTACH_COLOR0 + i;
		}
	}
}
static void null_framebuffer_destroy(Framebuffer *framebuffer) { }
static void null_framebuffer_viewport(Framebuffer *framebuffer, FloatRect vp) { }
static void null_framebuffer_viewport_current(Framebuffer *framebuffer, FloatRect *vp) { *vp = default_fb_viewport; }
static void null_framebuffer(Framebuffer *framebuffer) { }
static Framebuffer* null_framebuffer_current(void) { return (void*)&placeholder; }
static void null_framebuffer_clear(Framebuffer *framebuffer, BufferKindFlags flags, const Color *colorval, float depthval) { }
static void null_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) { }
static IntExtent null_framebuffer_get_size(Framebuffer *framebuffer) { return (IntExtent) { 64, 64 }; }

static void null_framebuffer_read_async(Framebuffer *framebuffer, FramebufferAttachment attachment, IntRect region, void *userdata, FramebufferReadAsyncCallback callback) {
	callback(NULL, userdata);
}

static int64_t null_vertex_buffer_stream_seek(SDL_RWops *rw, int64_t offset, int whence) { return 0; }
static int64_t null_vertex_buffer_stream_size(SDL_RWops *rw) { return (1 << 16); }
static size_t null_vertex_buffer_stream_write(SDL_RWops *rw, const void *data, size_t size, size_t num) { return num; }
static int null_vertex_buffer_stream_close(SDL_RWops *rw) { return 0; }

static size_t null_vertex_buffer_stream_read(SDL_RWops *rw, void *data, size_t size, size_t num) {
	SDL_SetError("Stream is write-only");
	return 0;
}

static SDL_RWops dummy_stream = {
	.seek = null_vertex_buffer_stream_seek,
	.size = null_vertex_buffer_stream_size,
	.write = null_vertex_buffer_stream_write,
	.read = null_vertex_buffer_stream_read,
	.close = null_vertex_buffer_stream_close,
};

static SDL_RWops* null_vertex_buffer_get_stream(VertexBuffer *vbuf) {
	return &dummy_stream;
}

static VertexBuffer* null_vertex_buffer_create(size_t capacity, void *data) { return (void*)&placeholder; }
static void null_vertex_buffer_set_debug_label(VertexBuffer *vbuf, const char *label) { }
static const char* null_vertex_buffer_get_debug_label(VertexBuffer *vbuf) { return "null vertex buffer"; }
static void null_vertex_buffer_destroy(VertexBuffer *vbuf) { }
static void null_vertex_buffer_invalidate(VertexBuffer *vbuf) { }

static IndexBuffer* null_index_buffer_create(uint index_size, size_t max_elements) { return (void*)&placeholder; }
static size_t null_index_buffer_get_capacity(IndexBuffer *ibuf) { return UINT32_MAX; }
static uint null_index_buffer_get_index_size(IndexBuffer *ibuf) { return sizeof(uint32_t); }
static const char* null_index_buffer_get_debug_label(IndexBuffer *ibuf) { return "null index buffer"; }
static void null_index_buffer_set_debug_label(IndexBuffer *ibuf, const char *label) { }
static void null_index_buffer_set_offset(IndexBuffer *ibuf, size_t offset) { }
static size_t null_index_buffer_get_offset(IndexBuffer *ibuf) { return 0; }
static void null_index_buffer_add_indices(IndexBuffer *ibuf, size_t data_size, void *data) { }
static void null_index_buffer_destroy(IndexBuffer *ibuf) { }
static void null_index_buffer_invalidate(IndexBuffer *ibuf) { }

static VertexArray* null_vertex_array_create(void) { return (void*)&placeholder; }
static void null_vertex_array_set_debug_label(VertexArray *varr, const char *label) { }
static const char* null_vertex_array_get_debug_label(VertexArray *varr) { return "null vertex array"; }
static void null_vertex_array_destroy(VertexArray *varr) { }
static void null_vertex_array_attach_vertex_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) { }
static void null_vertex_array_attach_index_buffer(VertexArray *varr, IndexBuffer *vbuf) { }
static VertexBuffer* null_vertex_array_get_vertex_attachment(VertexArray *varr, uint attachment) { return (void*)&placeholder; }
static IndexBuffer* null_vertex_array_get_index_attachment(VertexArray *varr) { return (void*)&placeholder; }
static void null_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) { }

static void null_scissor(IntRect scissor) { }
static void null_scissor_current(IntRect *scissor) { *scissor = (IntRect) { 0 }; }

static void null_vsync(VsyncMode mode) { }
static VsyncMode null_vsync_current(void) { return VSYNC_NONE; }

static void null_swap(SDL_Window *window) { }

RendererBackend _r_backend_null = {
	.name = "null",
	.funcs = {
		.init = null_init,
		.post_init = null_post_init,
		.shutdown = null_shutdown,
		.create_window = null_create_window,
		.features = null_features,
		.capabilities = null_capabilities,
		.capabilities_current = null_capabilities_current,
		.draw = null_draw,
		.draw_indexed = null_draw,
		.color4 = null_color4,
		.color_current = null_color_current,
		.blend = null_blend,
		.blend_current = null_blend_current,
		.cull = null_cull,
		.cull_current = null_cull_current,
		.depth_func = null_depth_func,
		.depth_func_current = null_depth_func_current,
		.shader_language_supported = null_shader_language_supported,
		.shader_object_compile = null_shader_object_compile,
		.shader_object_destroy = null_shader_object_destroy,
		.shader_object_set_debug_label = null_shader_object_set_debug_label,
		.shader_object_get_debug_label = null_shader_object_get_debug_label,
		.shader_object_transfer = null_shader_object_transfer,
		.shader_program_link = null_shader_program_link,
		.shader_program_destroy = null_shader_program_destroy,
		.shader_program_set_debug_label = null_shader_program_set_debug_label,
		.shader_program_get_debug_label = null_shader_program_get_debug_label,
		.shader_program_transfer = null_shader_program_transfer,
		.shader = null_shader,
		.shader_current = null_shader_current,
		.shader_uniform = null_shader_uniform,
		.uniform = null_uniform,
		.uniform_type = null_uniform_type,
		.texture_create = null_texture_create,
		.texture_get_params = null_texture_get_params,
		.texture_get_size = null_texture_get_size,
		.texture_get_debug_label = null_texture_get_debug_label,
		.texture_set_debug_label = null_texture_set_debug_label,
		.texture_set_filter = null_texture_set_filter,
		.texture_set_wrap = null_texture_set_wrap,
		.texture_destroy = null_texture_destroy,
		.texture_invalidate = null_texture_invalidate,
		.texture_fill = null_texture_fill,
		.texture_fill_region = null_texture_fill_region,
		.texture_dump = null_texture_dump,
		.texture_clear = null_texture_clear,
		.texture_type_query = null_texture_type_query,
		.texture_transfer = null_texture_transfer,
		.framebuffer_create = null_framebuffer_create,
		.framebuffer_get_debug_label = null_framebuffer_get_debug_label,
		.framebuffer_set_debug_label = null_framebuffer_set_debug_label,
		.framebuffer_destroy = null_framebuffer_destroy,
		.framebuffer_attach = null_framebuffer_attach,
		.framebuffer_query_attachment = null_framebuffer_query_attachment,
		.framebuffer_outputs = null_framebuffer_outputs,
		.framebuffer_viewport = null_framebuffer_viewport,
		.framebuffer_viewport_current = null_framebuffer_viewport_current,
		.framebuffer = null_framebuffer,
		.framebuffer_current = null_framebuffer_current,
		.framebuffer_clear = null_framebuffer_clear,
		.framebuffer_copy = null_framebuffer_copy,
		.framebuffer_get_size = null_framebuffer_get_size,
		.framebuffer_read_async = null_framebuffer_read_async,
		.vertex_buffer_create = null_vertex_buffer_create,
		.vertex_buffer_get_debug_label = null_vertex_buffer_get_debug_label,
		.vertex_buffer_set_debug_label = null_vertex_buffer_set_debug_label,
		.vertex_buffer_destroy = null_vertex_buffer_destroy,
		.vertex_buffer_invalidate = null_vertex_buffer_invalidate,
		.vertex_buffer_get_stream = null_vertex_buffer_get_stream,
		.index_buffer_create = null_index_buffer_create,
		.index_buffer_get_capacity = null_index_buffer_get_capacity,
		.index_buffer_get_index_size = null_index_buffer_get_index_size,
		.index_buffer_get_debug_label = null_index_buffer_get_debug_label,
		.index_buffer_set_debug_label = null_index_buffer_set_debug_label,
		.index_buffer_set_offset = null_index_buffer_set_offset,
		.index_buffer_get_offset = null_index_buffer_get_offset,
		.index_buffer_add_indices = null_index_buffer_add_indices,
		.index_buffer_invalidate = null_index_buffer_invalidate,
		.index_buffer_destroy = null_index_buffer_destroy,
		.vertex_array_create = null_vertex_array_create,
		.vertex_array_get_debug_label = null_vertex_array_get_debug_label,
		.vertex_array_set_debug_label = null_vertex_array_set_debug_label,
		.vertex_array_destroy = null_vertex_array_destroy,
		.vertex_array_layout = null_vertex_array_layout,
		.vertex_array_attach_vertex_buffer = null_vertex_array_attach_vertex_buffer,
		.vertex_array_get_vertex_attachment = null_vertex_array_get_vertex_attachment,
		.vertex_array_attach_index_buffer = null_vertex_array_attach_index_buffer,
		.vertex_array_get_index_attachment = null_vertex_array_get_index_attachment,
		.scissor = null_scissor,
		.scissor_current = null_scissor_current,
		.vsync = null_vsync,
		.vsync_current = null_vsync_current,
		.swap = null_swap,
	},
};
