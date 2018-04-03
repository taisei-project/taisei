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
void null_shutdown() { }

bool null_supports(RendererFeature feature) {
	return true;
}

void null_capability(RendererCapability cap, bool value) { }
bool null_capability_current(RendererCapability cap) { return true; }

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

void null_texture_fill(Texture *tex, void *image_data) { }
void null_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data) { }
void null_texture_invalidate(Texture *tex) { }
void null_texture_replace(Texture *tex, TextureType type, uint w, uint h, void *image_data) { }

void null_texture_destroy(Texture *tex) {
	memset(tex, 0, sizeof(Texture));
}

void null_texture(uint unit, Texture *tex) { }
Texture* null_texture_current(uint unit) { return (void*)&placeholder; }

struct RenderTargetImpl {
	Texture *attachments[RENDERTARGET_MAX_ATTACHMENTS];
};

void null_target_create(RenderTarget *target) {
	target->impl = calloc(1, sizeof(RenderTargetImpl));
}

void null_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment) {
	target->impl->attachments[attachment] = tex;
}

Texture* null_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment) {
	return target->impl->attachments[attachment];
}

void null_target_destroy(RenderTarget *target) {
	free(target->impl);
	memset(target, 0, sizeof(RenderTarget));
}

void null_target(RenderTarget *target) { }
RenderTarget* null_target_current(void) { return (void*)&placeholder; }

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

void null_viewport_rect(IntRect rect) { }
void null_viewport_current(IntRect *out_rect) {
	out_rect->x = 0;
	out_rect->y = 0;
	out_rect->w = 800;
	out_rect->h = 600;
}

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
		.capability = null_capability,
		.capability_current = null_capability_current,
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
		.texture_destroy = null_texture_destroy,
		.texture_invalidate = null_texture_invalidate,
		.texture_fill = null_texture_fill,
		.texture_fill_region = null_texture_fill_region,
		.texture_replace = null_texture_replace,
		.texture = null_texture,
		.texture_current = null_texture_current,
		.target_create = null_target_create,
		.target_destroy = null_target_destroy,
		.target_attach = null_target_attach,
		.target_get_attachment = null_target_get_attachment,
		.target = null_target,
		.target_current = null_target_current,
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
		.viewport_rect = null_viewport_rect,
		.viewport_current = null_viewport_current,
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
