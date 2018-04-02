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

char placeholder;

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	return SDL_CreateWindow(title, x, y, w, h, flags);
}

void _r_init(void) { }
void _r_shutdown() { }

bool r_supports(RendererFeature feature) {
	return true;
}

void r_capability(RendererCapability cap, bool value) { }
bool r_capability_current(RendererCapability cap) { return true; }

void r_color4(float r, float g, float b, float a) { }
Color r_color_current(void) { return rgba(0, 0, 0, 0); }

void r_blend(BlendMode mode) { }
BlendMode r_blend_current(void) { return BLEND_NONE; }

void r_cull(CullFaceMode mode) { }
CullFaceMode r_cull_current(void) { return CULL_BACK; }

void r_depth_func(DepthTestFunc func) { }
DepthTestFunc r_depth_func_current(void) { return DEPTH_LESS; }

void r_shader_ptr(ShaderProgram *prog) { }
ShaderProgram* r_shader_current(void) { return (void*)&placeholder; }

Uniform* r_shader_uniform(ShaderProgram *prog, const char *uniform_name) {
	return (void*)&placeholder;
}

void r_uniform_ptr(Uniform *uniform, uint count, const void *data) { }

UniformType r_uniform_type(Uniform *uniform) { return UNIFORM_FLOAT; }

void r_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances, uint base_instance) { }

void r_texture_create(Texture *tex, const TextureParams *params) {
	tex->w = params->width;
	tex->h = params->height;
	tex->type = params->type;
	tex->impl = (void*)&placeholder;
}

void r_texture_fill(Texture *tex, void *image_data) { }
void r_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data) { }
void r_texture_invalidate(Texture *tex) { }
void r_texture_replace(Texture *tex, TextureType type, uint w, uint h, void *image_data) { }

void r_texture_destroy(Texture *tex) {
	memset(tex, 0, sizeof(Texture));
}

void r_texture_ptr(uint unit, Texture *tex) { }
Texture* r_texture_current(uint unit) { return (void*)&placeholder; }

struct RenderTargetImpl {
	Texture *attachments[RENDERTARGET_MAX_ATTACHMENTS];
};

void r_target_create(RenderTarget *target) {
	target->impl = calloc(1, sizeof(RenderTargetImpl));
}

void r_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment) {
	target->impl->attachments[attachment] = tex;
}

Texture* r_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment) {
	return target->impl->attachments[attachment];
}

void r_target_destroy(RenderTarget *target) {
	free(target->impl);
	memset(target, 0, sizeof(RenderTarget));
}

void r_target(RenderTarget *target) { }
RenderTarget* r_target_current(void) { return (void*)&placeholder; }

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data) {
	vbuf->offset = 0;
	vbuf->size = capacity;
	vbuf->impl = (void*)&placeholder;
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	memset(vbuf, 0, sizeof(VertexBuffer));
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	vbuf->offset = 0;
}

void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) { }

void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	vbuf->offset += data_size;
}

void r_vertex_array_create(VertexArray *varr) {
	varr->impl = (void*)&placeholder;
}

void r_vertex_array_destroy(VertexArray *varr) {
	memset(varr, 0, sizeof(VertexArray));
}

void r_vertex_array_attach_buffer(VertexArray *varr, VertexBuffer *vbuf, uint attachment) { }

VertexBuffer* r_vertex_array_get_attachment(VertexArray *varr, uint attachment) {
	return (void*)&placeholder;
}

void r_vertex_array_layout(VertexArray *varr, uint nattribs, VertexAttribFormat attribs[nattribs]) { }

void r_vertex_array(VertexArray *varr) { }
VertexArray* r_vertex_array_current(void) { return (void*)&placeholder; }

void r_clear(ClearBufferFlags flags) { }
void r_clear_color4(float r, float g, float b, float a) { }
Color r_clear_color_current(void) { return rgba(0, 0, 0, 0); }

void r_viewport_rect(IntRect rect) { }
void r_viewport_current(IntRect *out_rect) {
	out_rect->x = 0;
	out_rect->y = 0;
	out_rect->w = 800;
	out_rect->h = 600;
}

void r_vsync(VsyncMode mode) { }
VsyncMode r_vsync_current(void) { return VSYNC_NONE; }

void r_swap(SDL_Window *window) { }

uint8_t* r_screenshot(uint *out_width, uint *out_height) { return NULL; }

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

ResourceHandler shader_program_res_handler = {
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

ResourceHandler shader_object_res_handler = {
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
