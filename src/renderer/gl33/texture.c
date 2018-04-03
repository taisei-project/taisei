/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "texture.h"
#include "../api.h"
#include "opengl.h"
#include "core.h"

static GLuint r_filter_to_gl_filter(TextureFilterMode mode) {
	static GLuint map[] = {
		[TEX_FILTER_DEFAULT] = GL_LINEAR,
		[TEX_FILTER_LINEAR]  = GL_LINEAR,
		[TEX_FILTER_NEAREST] = GL_NEAREST,
	};

	assert((uint)mode < sizeof(map)/sizeof(*map));
	return map[mode];
}

static GLuint r_wrap_to_gl_wrap(TextureWrapMode mode) {
	static GLuint map[] = {
		[TEX_WRAP_DEFAULT]   = GL_REPEAT,
		[TEX_WRAP_CLAMP]     = GL_CLAMP_TO_EDGE,
		[TEX_WRAP_MIRROR]    = GL_MIRRORED_REPEAT,
		[TEX_WRAP_REPEAT]    = GL_REPEAT,
	};

	assert((uint)mode < sizeof(map)/sizeof(*map));
	return map[mode];
}

typedef struct TexTypeInfo {
	GLuint internal_fmt;
	GLuint external_fmt;
	size_t pixel_size;
} TexTypeInfo;

static TexTypeInfo* tex_type_info(TextureType type) {
	static TexTypeInfo map[] = {
		[TEX_TYPE_DEFAULT]   = { GL_RGBA8,             GL_RGBA,            4 },
		[TEX_TYPE_RGB]       = { GL_RGB8,              GL_RGBA,            3 },
		[TEX_TYPE_RGBA]      = { GL_RGBA8,             GL_RGBA,            4 },
		[TEX_TYPE_DEPTH]     = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, 1 },
	};

	assert((uint)type < sizeof(map)/sizeof(*map));
	return map + type;
}

static void gl33_texture_set_assume_active(Texture *tex, void *image_data) {
	assert(tex->impl != NULL);

	gl33_bind_pbo(tex->impl->pbo);

	if(tex->impl->pbo) {
		size_t s = tex->w * tex->h * tex_type_info(tex->type)->pixel_size;
		glBufferData(GL_PIXEL_UNPACK_BUFFER, s, image_data, GL_STREAM_DRAW);
		image_data = NULL;
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		tex->impl->fmt_internal,
		tex->w,
		tex->h,
		0,
		tex->impl->fmt_external,
		GL_UNSIGNED_BYTE,
		image_data
	);

	gl33_bind_pbo(0);
}

static void gl33_texture_set(Texture *tex, void *image_data) {
	assert(tex->impl != NULL);

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit);
	gl33_texture_set_assume_active(tex, image_data);
	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_create(Texture *tex, const TextureParams *params) {
	memset(tex, 0, sizeof(Texture));

	tex->w = params->width;
	tex->h = params->height;
	tex->type = params->type;
	tex->impl = calloc(1, sizeof(TextureImpl));

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);

	glGenTextures(1, &tex->impl->gl_handle);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit);

	// TODO: mipmaps

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(params->wrap.s));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(params->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(params->filter.downscale));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(params->filter.upscale));

	tex->impl->fmt_internal = tex_type_info(params->type)->internal_fmt;
	tex->impl->fmt_external = tex_type_info(params->type)->external_fmt;

	if(params->stream) {
		glGenBuffers(1, &tex->impl->pbo);
	}

	gl33_texture_set_assume_active(tex, params->image_data);
	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_invalidate(Texture *tex) {
	gl33_texture_set(tex, NULL);
}

void gl33_texture_fill(Texture *tex, void *image_data) {
	// r_texture_fill_region(tex, 0, 0, tex->w, tex->h, image_data);
	gl33_texture_set(tex, image_data);
}

void gl33_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data) {
	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit);
	glTexSubImage2D(
		GL_TEXTURE_2D, 0,
		x, y, w, h,
		tex_type_info(tex->type)->external_fmt,
		GL_UNSIGNED_BYTE,
		image_data
	);
	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_replace(Texture *tex, TextureType type, uint w, uint h, void *image_data) {
	assert(tex->impl != NULL);

	tex->w = w;
	tex->h = h;

	if(type != tex->type) {
		tex->type = type;
		tex->impl->fmt_internal = tex_type_info(type)->internal_fmt;
		tex->impl->fmt_external = tex_type_info(type)->external_fmt;
	}

	gl33_texture_set(tex, image_data);
}

void gl33_texture_destroy(Texture *tex) {
	if(tex->impl) {
		gl33_texture_deleted(tex);

		glDeleteTextures(1, &tex->impl->gl_handle);

		if(tex->impl->pbo) {
			glDeleteBuffers(1, &tex->impl->pbo);
		}

		free(tex->impl);
		tex->impl = NULL;
	}
}
