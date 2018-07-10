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
		[TEX_FILTER_LINEAR]  = GL_LINEAR,
		[TEX_FILTER_LINEAR_MIPMAP_NEAREST]  = GL_LINEAR_MIPMAP_NEAREST,
		[TEX_FILTER_LINEAR_MIPMAP_LINEAR]  = GL_LINEAR_MIPMAP_LINEAR,
		[TEX_FILTER_NEAREST] = GL_NEAREST,
		[TEX_FILTER_NEAREST_MIPMAP_NEAREST]  = GL_NEAREST_MIPMAP_NEAREST,
		[TEX_FILTER_NEAREST_MIPMAP_LINEAR]  = GL_NEAREST_MIPMAP_LINEAR,
	};

	assert((uint)mode < sizeof(map)/sizeof(*map));
	return map[mode];
}

static GLuint r_wrap_to_gl_wrap(TextureWrapMode mode) {
	static GLuint map[] = {
		[TEX_WRAP_CLAMP]     = GL_CLAMP_TO_EDGE,
		[TEX_WRAP_MIRROR]    = GL_MIRRORED_REPEAT,
		[TEX_WRAP_REPEAT]    = GL_REPEAT,
	};

	assert((uint)mode < sizeof(map)/sizeof(*map));
	return map[mode];
}

GLTextureTypeInfo* gl33_texture_type_info(TextureType type) {
	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R]         = { GL_R8,                GL_RED,             GL_UNSIGNED_BYTE,  1 },
		[TEX_TYPE_RG]        = { GL_RG8,               GL_RG,              GL_UNSIGNED_BYTE,  2 },
		[TEX_TYPE_RGB]       = { GL_RGB8,              GL_RGB,             GL_UNSIGNED_BYTE,  3 },
		[TEX_TYPE_RGBA]      = { GL_RGBA8,             GL_RGBA,            GL_UNSIGNED_BYTE,  4 },
		[TEX_TYPE_DEPTH]     = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE,  1 },
	};

	assert((uint)type < sizeof(map)/sizeof(*map));
	return map + type;
}

static void gl33_texture_set_assume_active(Texture *tex, uint mipmap, void *image_data) {
	assert(tex->impl != NULL);
	assert(mipmap < tex->impl->params.mipmaps);

	gl33_bind_pbo(tex->impl->pbo);

	if(tex->impl->pbo) {
		size_t s = tex->w * tex->h * GLVT.texture_type_info(tex->type)->pixel_size;
		glBufferData(GL_PIXEL_UNPACK_BUFFER, s, image_data, GL_STREAM_DRAW);
		image_data = NULL;
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		mipmap,
		tex->impl->type_info->internal_fmt,
		max(1, floor(tex->w / pow(2, mipmap))),
		max(1, floor(tex->h / pow(2, mipmap))),
		0,
		tex->impl->type_info->external_fmt,
		tex->impl->type_info->component_type,
		image_data
	);

	gl33_bind_pbo(0);
	tex->impl->mipmaps_outdated = true;
}

static void gl33_texture_set(Texture *tex, uint mipmap, void *image_data) {
	assert(tex->impl != NULL);

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit, false);
	gl33_texture_set_assume_active(tex, mipmap, image_data);
	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_create(Texture *tex, const TextureParams *params) {
	memset(tex, 0, sizeof(Texture));

	tex->w = params->width;
	tex->h = params->height;
	tex->type = params->type;
	tex->impl = calloc(1, sizeof(TextureImpl));
	memcpy(&tex->impl->params, params, sizeof(*params));
	TextureParams *p = &tex->impl->params;

	uint max_mipmaps = 1 + floor(log2(max(tex->w, tex->h)));

	if(p->mipmaps == 0) {
		if(p->mipmap_mode == TEX_MIPMAP_AUTO) {
			p->mipmaps = TEX_MIPMAPS_MAX;
		} else {
			p->mipmaps = 1;
		}
	}

	if(p->mipmaps == TEX_MIPMAPS_MAX || p->mipmaps > max_mipmaps) {
		p->mipmaps = max_mipmaps;
	}

	if(p->anisotropy == 0) {
		p->anisotropy = TEX_ANISOTROPY_DEFAULT;
	}

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);

	glGenTextures(1, &tex->impl->gl_handle);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit, false);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(p->wrap.s));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(p->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(p->filter.min));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(p->filter.mag));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, p->mipmaps - 1);

	if(glext.texture_filter_anisotropic) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, p->anisotropy);
	}

	tex->impl->type_info = GLVT.texture_type_info(p->type);

	if(p->stream && glext.pixel_buffer_object) {
		glGenBuffers(1, &tex->impl->pbo);
	}

	uint width = p->width;
	uint height = p->height;

	for(uint i = 0; i < p->mipmaps; ++i) {
		assert(width == max(1, floor(tex->w / pow(2, i))));
		assert(height == max(1, floor(tex->h / pow(2, i))));

		glTexImage2D(
			GL_TEXTURE_2D,
			i,
			tex->impl->type_info->internal_fmt,
			width,
			height,
			0,
			tex->impl->type_info->external_fmt,
			tex->impl->type_info->component_type,
			NULL
		);

		width = max(1, floor(width / 2));
		height = max(1, floor(height / 2));
	}

	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_get_params(Texture *tex, TextureParams *params) {
	assert(tex->impl != NULL);
	memcpy(params, &tex->impl->params, sizeof(*params));
}

void gl33_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);

	if(tex->impl->params.filter.min != fmin) {
		gl33_sync_texunit(unit, false);
		tex->impl->params.filter.min = fmin;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(fmin));
	}

	if(tex->impl->params.filter.mag != fmag) {
		gl33_sync_texunit(unit, false);
		tex->impl->params.filter.mag = fmag;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(fmag));
	}

	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);

	if(tex->impl->params.wrap.s != ws) {
		gl33_sync_texunit(unit, false);
		tex->impl->params.wrap.s = ws;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(ws));
	}

	if(tex->impl->params.wrap.t != wt) {
		gl33_sync_texunit(unit, false);
		tex->impl->params.wrap.t = wt;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(wt));
	}

	r_texture_ptr(unit, prev_tex);
}

void gl33_texture_invalidate(Texture *tex) {
	for(uint i = 0; i < tex->impl->params.mipmaps; ++i) {
		gl33_texture_set(tex, i, NULL);
	}
}

void gl33_texture_fill(Texture *tex, uint mipmap, void *image_data) {
	assert(mipmap == 0 || tex->impl->params.mipmap_mode != TEX_MIPMAP_AUTO);
	gl33_texture_set(tex, mipmap, image_data);
}

void gl33_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data) {
	assert(mipmap == 0 || tex->impl->params.mipmap_mode != TEX_MIPMAP_AUTO);

	uint unit = gl33_active_texunit();
	Texture *prev_tex = r_texture_current(unit);
	r_texture_ptr(unit, tex);
	gl33_sync_texunit(unit, false);
	glTexSubImage2D(
		GL_TEXTURE_2D, mipmap,
		x, y, w, h,
		GLVT.texture_type_info(tex->type)->external_fmt,
		GL_UNSIGNED_BYTE,
		image_data
	);
	r_texture_ptr(unit, prev_tex);
	tex->impl->mipmaps_outdated = true;
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

void gl33_texture_taint(Texture *tex) {
	tex->impl->mipmaps_outdated = true;
}

void gl33_texture_prepare(Texture *tex) {
	assert(tex->impl != NULL);

	if(tex->impl->params.mipmap_mode == TEX_MIPMAP_AUTO && tex->impl->mipmaps_outdated) {
		log_debug("Generating mipmaps for texture %u", tex->impl->gl_handle);

		uint unit = gl33_active_texunit();
		Texture *prev_tex = r_texture_current(unit);
		r_texture_ptr(unit, tex);
		gl33_sync_texunit(unit, false);
		glGenerateMipmap(GL_TEXTURE_2D);
		r_texture_ptr(unit, prev_tex);
		tex->impl->mipmaps_outdated = false;
	}
}
