/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "color.h"
#include "resource/resource.h"
#include "resource/shader_program.h"
#include "resource/texture.h"

typedef enum MatrixMode {
	// XXX: Mimicking the gl matrix mode api for easy replacement.
	// But we might just want to add proper functions to modify the non modelview matrices.

	MM_MODELVIEW,
	MM_PROJECTION,
	MM_TEXTURE,
} MatrixMode;

typedef enum TextureType {
	TEX_TYPE_DEFAULT,
	TEX_TYPE_RGBA,
	TEX_TYPE_DEPTH,
} TextureType;

typedef enum TextureFilterMode {
	TEX_FILTER_DEFAULT,
	TEX_FILTER_NEAREST,
	TEX_FILTER_LINEAR,
} TextureFilterMode;

typedef enum TextureWrapMode {
	TEX_WRAP_DEFAULT,
	TEX_WRAP_REPEAT,
	TEX_WRAP_MIRROR,
	TEX_WRAP_CLAMP,
} TextureWrapMode;

typedef struct TextureParams {
	uint width;
	uint height;
	TextureType type;

	struct {
		TextureFilterMode upscale;
		TextureFilterMode downscale;
	} filter;

	struct {
		TextureWrapMode s;
		TextureWrapMode t;
	} wrap;

	uint mipmaps;
	uint8_t *image_data;
} TextureParams attr_designated_init;

typedef struct TextureImpl TextureImpl;

typedef struct Texture {
	uint w;
	uint h;
	TextureType type;
	TextureImpl *impl;
} Texture;

typedef enum RenderTargetAttachment {
	RENDERTARGET_ATTACHMENT_DEPTH,
	RENDERTARGET_ATTACHMENT_COLOR0,
	RENDERTARGET_ATTACHMENT_COLOR1,
	RENDERTARGET_ATTACHMENT_COLOR2,
	RENDERTARGET_ATTACHMENT_COLOR3,
} RenderTargetAttachment;

enum {
	RENDERTARGET_MAX_COLOR_ATTACHMENTS = 4,
	RENDERTARGET_MAX_ATTACHMENTS = RENDERTARGET_ATTACHMENT_COLOR0 + RENDERTARGET_MAX_COLOR_ATTACHMENTS,
};

typedef struct RenderTargetImpl RenderTargetImpl;

typedef struct RenderTarget {
	Texture *attachments[RENDERTARGET_MAX_ATTACHMENTS];
	RenderTargetImpl *impl;
} RenderTarget;

typedef enum UniformType {
	UNIFORM_FLOAT,
	UNIFORM_VEC2,
	UNIFORM_VEC3,
	UNIFORM_VEC4,
	UNIFORM_INT,
	UNIFORM_IVEC2,
	UNIFORM_IVEC3,
	UNIFORM_IVEC4,
	UNIFORM_SAMPLER,
	UNIFORM_MAT3,
	UNIFORM_MAT4,
	UNIFORM_UNKNOWN,
} UniformType;

typedef struct Uniform Uniform;

/*
 * Creates an SDL window with proper flags, and if needed, sets up context and the r_* pointers.
 * Must be called before anything else.
 */

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags)
	attr_nonnull(1) attr_nodiscard;

/*
 *	TODO: Document these.
 */

void r_init(void);
void r_shutdown(void);

void r_mat_mode(MatrixMode mode);
void r_mat_push(void);
void r_mat_pop(void);
void r_mat_identity(void);
void r_mat_translate_v(vec3 v);
void r_mat_rotate_v(float angle, vec3 v);
void r_mat_scale_v(vec3 v);
void r_mat_ortho(float left, float right, float bottom, float top, float near, float far);
void r_mat_perspective(float angle, float aspect, float near, float far);

void r_color4(float r, float g, float b, float a);

void r_shader_ptr(ShaderProgram *prog) attr_nonnull(1);
void r_shader_standard(void);
void r_shader_standard_notex(void);
ShaderProgram* r_shader_current(void) attr_returns_nonnull;

Uniform* r_shader_uniform(ShaderProgram *prog, const char *uniform_name) attr_nonnull(1, 2);
void r_uniform_ptr(Uniform *uniform, uint count, const void *data) attr_nonnull(3);

void r_flush(void);
void r_draw_quad(void);
void r_draw_model(const char *model);

void r_texture_create(Texture *tex, const TextureParams *params) attr_nonnull(1, 2);
void r_texture_fill(Texture *tex, void *image_data) attr_nonnull(1, 2);
void r_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data) attr_nonnull(1, 6);
void r_texture_destroy(Texture *tex) attr_nonnull(1);

void r_texture_ptr(uint unit, Texture *tex);
Texture* r_texture_current(uint unit);

void r_target_create(RenderTarget *target) attr_nonnull(1);
void r_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment) attr_nonnull(1);
Texture* r_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment) attr_nonnull(1);
void r_target_destroy(RenderTarget *target) attr_nonnull(1);

void r_target(RenderTarget *target);
RenderTarget* r_target_current(void);

/*
 *	Provided by the API module
 */

static inline attr_must_inline
ShaderProgram* r_shader_get(const char *name) {
	return get_resource_data(RES_SHADER_PROGRAM, name, RESF_DEFAULT | RESF_UNSAFE);
}

static inline attr_must_inline
ShaderProgram* r_shader_get_optional(const char *name) {
	ShaderProgram *prog = get_resource_data(RES_SHADER_PROGRAM, name, RESF_OPTIONAL | RESF_UNSAFE);

	if(!prog) {
		log_warn("shader program %s could not be loaded", name);
	}

	return prog;
}

static inline attr_must_inline
Texture* r_texture_get(const char *name) {
	return get_resource_data(RES_TEXTURE, name, RESF_DEFAULT | RESF_UNSAFE);
}

static inline attr_must_inline
void r_mat_translate(float x, float y, float z) {
	r_mat_translate_v((vec3) { x, y, z });
}

static inline attr_must_inline
void r_mat_rotate(float angle, float nx, float ny, float nz) {
	r_mat_rotate_v(angle, (vec3) { nx, ny, nz });
}

static inline attr_must_inline
void r_mat_rotate_deg(float angle_degrees, float nx, float ny, float nz) {
	r_mat_rotate_v(angle_degrees * 0.017453292519943295, (vec3) { nx, ny, nz });
}

static inline attr_must_inline
void r_mat_rotate_deg_v(float angle_degrees, vec3 v) {
	r_mat_rotate_v(angle_degrees * 0.017453292519943295, v);
}

static inline attr_must_inline
void r_mat_scale(float sx, float sy, float sz) {
	r_mat_scale_v((vec3) { sx, sy, sz });
}

static inline attr_must_inline
void r_color(Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_color4(r, g, b, a);
}

static inline attr_must_inline
void r_color3(float r, float g, float b) {
	r_color4(r, g, b, 1.0);
}

static inline attr_must_inline attr_nonnull(1)
void r_shader(const char *prog) {
	r_shader_ptr(r_shader_get(prog));
}

static inline attr_must_inline
void r_texture(uint unit, const char *tex) {
	r_texture_ptr(unit, r_texture_get(tex));
}

static inline attr_must_inline
Uniform* r_shader_current_uniform(const char *name) {
	return r_shader_uniform(r_shader_current(), name);
}

static inline attr_must_inline
void r_uniform(const char *name, uint count, void *data) {
	r_uniform_ptr(r_shader_current_uniform(name), count, data);
}

static inline attr_must_inline
void r_uniform_float(const char *name, float v) {
	r_uniform(name, 1, (float[1]) { v });
}

static inline attr_must_inline
void r_uniform_int(const char *name, int v) {
	r_uniform(name, 1, (int[1]) { v });
}

static inline attr_must_inline
void r_uniform_vec2(const char *name, float x, float y) {
	r_uniform(name, 1, (float[2]) { x, y } );
}

static inline attr_must_inline
void r_uniform_vec3(const char *name, float x, float y, float z) {
	r_uniform(name, 1, (vec3) { x, y, z });
}

static inline attr_must_inline
void r_uniform_vec4(const char *name, float x, float y, float z, float w) {
	r_uniform(name, 1, (vec4) { x, y, z, w });
}

static inline attr_must_inline
void r_uniform_rgb(const char *name, Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_uniform_vec3(name, r, g, b);
}

static inline attr_must_inline
void r_uniform_rgba(const char *name, Color c) {
	static float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	r_uniform_vec4(name, r, g, b, a);
}

static inline attr_must_inline
void r_uniform_complex(const char *name, complex z) {
	r_uniform(name, 1, (float[]) { creal(z), cimag(z) });
}
