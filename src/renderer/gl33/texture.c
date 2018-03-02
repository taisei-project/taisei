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

void r_texture_create(Texture *tex, const TextureParams *params) {
	tex->w = params->width;
	tex->h = params->height;
	tex->impl = calloc(1, sizeof(TextureImpl));
	tex->impl->wrap.s = params->wrap.s;
	tex->impl->wrap.t = params->wrap.t;
	tex->impl->filter.downscale = params->filter.downscale;
	tex->impl->filter.upscale = params->filter.upscale;

	uint unit = gl33_active_texunit();
	const Texture *prev_tex = r_texture_current(unit);

	glGenTextures(1, &tex->impl->gl_handle);
	r_texture_ptr(unit, tex);
	r_flush();

	// TODO: mipmaps

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(tex->impl->wrap.s));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(tex->impl->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(tex->impl->filter.downscale));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(tex->impl->filter.upscale));

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, params->image_data);

	r_texture_ptr(unit, prev_tex);
}

void r_texture_fill(Texture *tex, uint8_t *image_data) {
	uint unit = gl33_active_texunit();
	const Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	r_flush();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	r_texture_ptr(unit, prev_tex);
}

void r_texture_make_render_target(Texture *tex) {
	assert(tex != NULL);
	assert(tex->impl != NULL);

	if(tex->impl->fbo.gl_handle) {
		return;
	}

	assert(tex->impl->fbo.gl_depthtex_handle == 0);

	uint unit = gl33_active_texunit();
	const Texture *prev_tex = r_texture_current(unit);

	Texture depth_tex = { .impl = &(TextureImpl){ 0 }};
	glGenTextures(1, &depth_tex.impl->gl_handle);

	r_texture_ptr(unit, &depth_tex);
	r_flush();

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(tex->impl->wrap.s));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(tex->impl->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(tex->impl->filter.downscale));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(tex->impl->filter.upscale));

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex->w, tex->h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glGenFramebuffers(1, &tex->impl->fbo.gl_handle);
	const Texture *prev_target = r_target_current();
	r_target(tex);
	r_flush();

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->impl->gl_handle, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.impl->gl_handle, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	r_target(prev_target);
	r_texture_ptr(unit, prev_tex);
}

void r_texture_destroy(Texture *tex) {
	if(tex->impl) {
		if(tex->impl->fbo.gl_handle) {
			glDeleteFramebuffers(1, &tex->impl->fbo.gl_handle);
		}

		if(tex->impl->gl_handle) {
			glDeleteTextures(1, &tex->impl->gl_handle);
		}

		if(tex->impl->fbo.gl_depthtex_handle) {
			glDeleteTextures(1, &tex->impl->fbo.gl_depthtex_handle);
		}

		free(tex->impl);
		tex->impl = NULL;
	}
}
