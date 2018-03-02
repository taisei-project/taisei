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

/*
 * Provided by the backend at link time.
 * Creates an SDL window with proper flags, and if needed, sets up context and the r_* pointers.
 * Must be called before anything else.
 */

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags)
	attr_nonnull(1) attr_nodiscard;

/*
 *	Provided by the backend at run time.
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

void r_shader_ptr(const ShaderProgram *prog) attr_nonnull(1);
void r_shader_standard(void);
void r_shader_standard_notex(void);
const ShaderProgram* r_shader_current(void) attr_returns_nonnull;

void r_flush(void);
void r_draw_quad(void);
void r_draw_model(const char *model);

void r_texture_create(Texture *tex, const TextureParams *params);
void r_texture_fill(Texture *tex, uint8_t *image_data);
void r_texture_make_render_target(Texture *tex);
void r_texture_destroy(Texture *tex);

void r_texture_ptr(uint unit, const Texture *tex) attr_nonnull(2);
const Texture* r_texture_current(uint unit);

void r_target(const Texture *target);
const Texture* r_target_current(void);

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

static inline attr_must_inline attr_nonnull(2)
void r_texture(uint unit, const char *tex) {
	r_texture_ptr(unit, get_tex(tex));
}
