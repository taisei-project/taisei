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
#include "../common/opengl.h"
#include "core.h"

static inline GLuint r_filter_to_gl_filter(TextureFilterMode fmode) {
	// TODO: implement this
	return GL_LINEAR;
}

static inline GLuint r_wrap_to_gl_wrap(TextureWrapMode wmode) {
	// TODO: implement this
	return GL_REPEAT;
}

static inline GLuint r_type_to_gl_internal_format(TextureType type) {
	switch(type) {
		case TEX_TYPE_DEFAULT:
		case TEX_TYPE_RGBA:
			return GL_RGBA8;

		case TEX_TYPE_DEPTH:
			return GL_DEPTH_COMPONENT16;

		default:
			log_fatal("Bad texture type %x", type);
	}
}

static inline GLuint r_type_to_gl_external_format(TextureType type) {
	switch(type) {
		case TEX_TYPE_DEFAULT:
		case TEX_TYPE_RGBA:
			return GL_RGBA;

		case TEX_TYPE_DEPTH:
			return GL_DEPTH_COMPONENT;

		default:
			log_fatal("Bad texture type %x", type);
	}
}

void r_texture_create(Texture *tex, const TextureParams *params) {
	assert(tex != NULL);
	assert(params != NULL);

	memset(tex, 0, sizeof(Texture));

	tex->w = params->width;
	tex->h = params->height;
	tex->type = params->type;
	tex->impl = calloc(1, sizeof(TextureImpl));

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);

	glGenTextures(1, &tex->impl->gl_handle);
	r_texture_ptr(unit, tex);
	r_flush();

	// TODO: mipmaps

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(params->wrap.s));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(params->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(params->filter.downscale));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(params->filter.upscale));

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		r_type_to_gl_internal_format(params->type),
		tex->w,
		tex->h,
		0,
		r_type_to_gl_external_format(params->type),
		GL_UNSIGNED_BYTE,
		params->image_data
	);

	r_texture_ptr(unit, prev_tex);
}

void r_texture_fill(Texture *tex, void *image_data) {
	r_texture_fill_region(tex, 0, 0, tex->w, tex->h, image_data);
}

void r_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data) {
	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	r_flush();
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	r_texture_ptr(unit, prev_tex);
}

void r_texture_destroy(Texture *tex) {
	if(tex->impl) {
		glDeleteTextures(1, &tex->impl->gl_handle);
		free(tex->impl);
		tex->impl = NULL;
	}
}
