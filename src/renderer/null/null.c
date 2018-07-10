/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "../api.h"
#include "resource/shader_object.h"
#include "../common/backend.h"

char placeholder;

SDL_Window* null_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	return SDL_CreateWindow(title, x, y, w, h, flags);
}

void null_init(void) { }
void null_post_init(void) { }
void null_shutdown(void) { }

bool null_supports(RendererFeature feature) {
	return true;
}

void null_capabilities(r_capability_bits_t capbits) { }
r_capability_bits_t null_capabilities_current(void) { return (r_capability_bits_t)-1; }

void null_color4(float r, float g, float b, float a) { }
Color null_color_current(void) { return rgba(0, 0, 0, 0); }

void null_blend(BlendMode mode) { }
BlendMode null_blend_current(void) { return BLEND_NONE; }

void null_cull(CullFaceMode mode) { }
CullFaceMode null_cull_current(void) { return CULL_BACK; }

void null_depth_func(DepthTestFunc func) { }
DepthTestFunc null_depth_func_current(void) { return DEPTH_LESS; }

void null_shader(ShaderProgram *prog) { }
ShaderProgram* null_shader_current(void) { return (void*)&placeholder; }

Uniform* null_shader_uniform(ShaderProgram *prog, const char *uniform_name) {
	return (void*)&placeholder;
}

void null_uniform(Uniform *uniform, uint count, const void *data) { }

UniformType null_uniform_type(Uniform *uniform) { return UNIFORM_FLOAT; }

void null_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances, uint base_instance) { }

void null_texture_create(Texture *tex, const TextureParams *params) {
	tex->w = params->width;
	tex->h = params->height;
	tex->type = params->type;
	tex->impl = (void*)&placeholder;
}

void null_texture_get_params(Texture *tex, TextureParams *params) {
	memset(params, 0, sizeof(*params));
	params->width = tex->w;
	params->height = tex->h;
	params->type = tex->type;
}

void null_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) { }
void null_texture_set_wrap(Texture *tex, TextureWrapMode fmin, TextureWrapMode fmag) { }
void null_texture_fill(Texture *tex, uint mipmap, void *image_data) { }
void null_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data) { }
void null_texture_invalidate(Texture *tex) { }

void null_texture_destroy(Texture *tex) {
	memset(tex, 0, sizeof(Texture));
}

void null_texture(uint unit, Texture *tex) { }
Texture* null_texture_current(uint unit) { return (void*)&placeholder; }

struct FramebufferImpl {
	Texture *attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	IntRect viewport;
};

static IntRect default_fb_viewport;

void null_framebuffer_create(Framebuffer *framebuffer) {
	framebuffer->impl = calloc(1, sizeof(FramebufferImpl));
}

void null_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	framebuffer->impl->attachments[attachment] = tex;
}

Texture* null_framebuffer_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	return framebuffer->impl->attachments[attachment];
}

uint null_framebuffer_attachment_mipmap(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	return 0;
}

void null_framebuffer_destroy(Framebuffer *framebuffer) {
	free(framebuffer->impl);
	memset(framebuffer, 0, sizeof(Framebuffer));
}

void null_framebuffer_viewport(Framebuffer *framebuffer, IntRect vp) {
	if(framebuffer) {
		framebuffer->impl->viewport = vp;
	} else {
		default_fb_viewport = vp;
	}
}

void null_framebuffer_viewport_current(Framebuffer *framebuffer, IntRect *vp) {
	if(framebuffer) {
		*vp = framebuffer->impl->viewport;
	} else {
		*vp = default_fb_viewport;
	}
}

void null_framebuffer(Framebuffer *framebuffer) { }
Framebuffer* null_framebuffer_current(void) { return (void*)&placeholder; }

void null_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data) {
	vbuf->offset = 0;
	vbuf->size = capacity;
	vbuf->impl = (void*)&placeholder;
}

void null_vertex_buffer_destroy(VertexBuffer *vbuf) {
	memset(vbuf, 0, sizeof(VertexBuffer));
}

void null_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	vbuf->offset = 0;
}

void null_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) { }

void null_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	vbuf->offset += data_size;
}

void null_vertex_array_create(VertexArray *varr) {
	varr->impl = (void*)&placeholder;
}

void null_vertex_array_destroy(VertexArray *varr) {
	memset(varr, 0, sizeof(VertexArray));
}

void null_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) { }

VertexBuffer* null_vertex_array_get_attachment(VertexArray *varr, uint attachment) {
	return (void*)&placeholder;
}

void null_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) { }

void null_vertex_array(VertexArray *varr) { }
VertexArray* null_vertex_array_current(void) { return (void*)&placeholder; }

void null_clear(ClearBufferFlags flags) { }
void null_clear_color4(float r, float g, float b, float a) { }
Color null_clear_color_current(void) { return rgba(0, 0, 0, 0); }

void null_vsync(VsyncMode mode) { }
VsyncMode null_vsync_current(void) { return VSYNC_NONE; }

void null_swap(SDL_Window *window) { }

uint8_t* null_screenshot(uint *out_width, uint *out_height) { return NULL; }

/*
 * Resource API
 */

static char* shader_program_path(const char *name) {
	return strjoin(SHPROG_PATH_PREFIX, name, SHPROG_EXT, NULL);
}

static bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, SHPROG_PATH_PREFIX);
}

static void* load_shader_program_begin(const char *path, uint flags) {
	return &placeholder;
}

static void* load_shader_program_end(void *opaque, const char *path, uint flags) {
	return &placeholder;
}

static void unload_shader_program(void *vprog) { }

ResourceHandler null_shader_program_res_handler = {
	.type = RES_SHADER_PROGRAM,
	.typename = "shader program",
	.subdir = SHPROG_PATH_PREFIX,

	.procs = {
		.find = shader_program_path,
		.check = check_shader_program_path,
		.begin_load = load_shader_program_begin,
		.end_load = load_shader_program_end,
		.unload = unload_shader_program,
	},
};

static char* shader_object_path(const char *name) {
	return NULL;
}

static bool check_shader_object_path(const char *path) {
	return false;
}

ResourceHandler null_shader_object_res_handler = {
	.type = RES_SHADER_OBJECT,
	.typename = "shader object",
	.subdir = SHOBJ_PATH_PREFIX,

	.procs = {
		.find = shader_object_path,
		.check = check_shader_object_path,
		.begin_load = load_shader_program_begin,
		.end_load = load_shader_program_end,
		.unload = unload_shader_program,
	},
};

RendererBackend _r_backend_null = {
	.name = "null",
	.funcs = {
		.init = null_init,
		.post_init = null_post_init,
		.shutdown = null_shutdown,
		.create_window = null_create_window,
		.supports = null_supports,
		.capabilities = null_capabilities,
		.capabilities_current = null_capabilities_current,
		.draw = null_draw,
		.color4 = null_color4,
		.color_current = null_color_current,
		.blend = null_blend,
		.blend_current = null_blend_current,
		.cull = null_cull,
		.cull_current = null_cull_current,
		.depth_func = null_depth_func,
		.depth_func_current = null_depth_func_current,
		.shader = null_shader,
		.shader_current = null_shader_current,
		.shader_uniform = null_shader_uniform,
		.uniform = null_uniform,
		.uniform_type = null_uniform_type,
		.texture_create = null_texture_create,
		.texture_get_params = null_texture_get_params,
		.texture_set_filter = null_texture_set_filter,
		.texture_set_wrap = null_texture_set_wrap,
		.texture_destroy = null_texture_destroy,
		.texture_invalidate = null_texture_invalidate,
		.texture_fill = null_texture_fill,
		.texture_fill_region = null_texture_fill_region,
		.texture = null_texture,
		.texture_current = null_texture_current,
		.framebuffer_create = null_framebuffer_create,
		.framebuffer_destroy = null_framebuffer_destroy,
		.framebuffer_attach = null_framebuffer_attach,
		.framebuffer_get_attachment = null_framebuffer_attachment,
		.framebuffer_get_attachment_mipmap = null_framebuffer_attachment_mipmap,
		.framebuffer_viewport = null_framebuffer_viewport,
		.framebuffer_viewport_current = null_framebuffer_viewport_current,
		.framebuffer = null_framebuffer,
		.framebuffer_current = null_framebuffer_current,
		.vertex_buffer_create = null_vertex_buffer_create,
		.vertex_buffer_destroy = null_vertex_buffer_destroy,
		.vertex_buffer_invalidate = null_vertex_buffer_invalidate,
		.vertex_buffer_write = null_vertex_buffer_write,
		.vertex_buffer_append = null_vertex_buffer_append,
		.vertex_array_create = null_vertex_array_create,
		.vertex_array_destroy = null_vertex_array_destroy,
		.vertex_array_layout = null_vertex_array_layout,
		.vertex_array_attach_buffer = null_vertex_array_attach_buffer,
		.vertex_array_get_attachment = null_vertex_array_get_attachment,
		.vertex_array = null_vertex_array,
		.vertex_array_current = null_vertex_array_current,
		.clear = null_clear,
		.clear_color4 = null_clear_color4,
		.clear_color_current = null_clear_color_current,
		.vsync = null_vsync,
		.vsync_current = null_vsync_current,
		.swap = null_swap,
		.screenshot = null_screenshot,
	},
	.res_handlers = {
		.shader_object = &null_shader_object_res_handler,
		.shader_program = &null_shader_program_res_handler,
	},
};
