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
#include "../glcommon/debug.h"

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

void gl33_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) {
	if(mipmap >= tex->params.mipmaps) {
		mipmap = tex->params.mipmaps - 1;
	}

	if(mipmap == 0) {
		if(width != NULL) {
			*width = tex->params.width;
		}

		if(height != NULL) {
			*height = tex->params.height;
		}
	} else {
		if(width != NULL) {
			*width = max(1, floor(tex->params.width / pow(2, mipmap)));
		}

		if(height != NULL) {
			*height = max(1, floor(tex->params.height / pow(2, mipmap)));
		}
	}
}

static void gl33_texture_set(Texture *tex, uint mipmap, void *image_data) {
	assert(mipmap < tex->params.mipmaps);

	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);
	gl33_bind_pbo(tex->pbo);

	if(tex->pbo) {
		size_t s = tex->params.width * tex->params.height * GLVT.texture_type_info(tex->params.type)->pixel_size;
		glBufferData(GL_PIXEL_UNPACK_BUFFER, s, image_data, GL_STREAM_DRAW);
		image_data = NULL;
	}

	uint width, height;
	gl33_texture_get_size(tex, mipmap, &width, &height);

	glTexImage2D(
		GL_TEXTURE_2D,
		mipmap,
		tex->type_info->internal_fmt,
		width,
		height,
		0,
		tex->type_info->external_fmt,
		tex->type_info->component_type,
		image_data
	);

	gl33_bind_pbo(0);
	tex->mipmaps_outdated = true;
}

Texture* gl33_texture_create(const TextureParams *params) {
	Texture *tex = calloc(1, sizeof(Texture));
	memcpy(&tex->params, params, sizeof(*params));
	TextureParams *p = &tex->params;

	uint max_mipmaps = 1 + floor(log2(max(tex->params.width, tex->params.height)));

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

	glGenTextures(1, &tex->gl_handle);
	snprintf(tex->debug_label, sizeof(tex->debug_label), "Texture #%i", tex->gl_handle);
	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(p->wrap.s));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(p->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(p->filter.min));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(p->filter.mag));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, p->mipmaps - 1);

	if(glext.texture_filter_anisotropic) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, p->anisotropy);
	}

	tex->type_info = GLVT.texture_type_info(p->type);

	if(p->stream && glext.pixel_buffer_object) {
		glGenBuffers(1, &tex->pbo);
	}

	for(uint i = 0; i < p->mipmaps; ++i) {
		uint width, height;
		gl33_texture_get_size(tex, i, &width, &height);

		glTexImage2D(
			GL_TEXTURE_2D,
			i,
			tex->type_info->internal_fmt,
			width,
			height,
			0,
			tex->type_info->external_fmt,
			tex->type_info->component_type,
			NULL
		);
	}

	return tex;
}

void gl33_texture_get_params(Texture *tex, TextureParams *params) {
	memcpy(params, &tex->params, sizeof(*params));
}

void gl33_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	if(tex->params.filter.min != fmin) {
		gl33_bind_texture(tex, false);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.filter.min = fmin;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(fmin));
	}

	if(tex->params.filter.mag != fmag) {
		gl33_bind_texture(tex, false);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.filter.mag = fmag;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(fmag));
	}
}

void gl33_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	if(tex->params.wrap.s != ws) {
		gl33_bind_texture(tex, false);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.wrap.s = ws;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(ws));
	}

	if(tex->params.wrap.t != wt) {
		gl33_bind_texture(tex, false);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.wrap.t = wt;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(wt));
	}
}

void gl33_texture_invalidate(Texture *tex) {
	for(uint i = 0; i < tex->params.mipmaps; ++i) {
		gl33_texture_set(tex, i, NULL);
	}
}

void gl33_texture_fill(Texture *tex, uint mipmap, void *image_data) {
	assert(mipmap == 0 || tex->params.mipmap_mode != TEX_MIPMAP_AUTO);
	gl33_texture_set(tex, mipmap, image_data);
}

void gl33_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, uint w, uint h, void *image_data) {
	assert(mipmap == 0 || tex->params.mipmap_mode != TEX_MIPMAP_AUTO);

	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);

	glTexSubImage2D(
		GL_TEXTURE_2D, mipmap,
		x, y, w, h,
		GLVT.texture_type_info(tex->params.type)->external_fmt,
		GL_UNSIGNED_BYTE,
		image_data
	);

	tex->mipmaps_outdated = true;
}

void gl33_texture_destroy(Texture *tex) {
	gl33_texture_deleted(tex);

	glDeleteTextures(1, &tex->gl_handle);

	if(tex->pbo) {
		glDeleteBuffers(1, &tex->pbo);
	}

	free(tex);
}

void gl33_texture_taint(Texture *tex) {
	tex->mipmaps_outdated = true;
}

void gl33_texture_prepare(Texture *tex) {
	if(tex->params.mipmap_mode == TEX_MIPMAP_AUTO && tex->mipmaps_outdated) {
		log_debug("Generating mipmaps for texture %u", tex->gl_handle);

		gl33_bind_texture(tex, false);
		gl33_sync_texunit(tex->binding_unit, false, true);
		glGenerateMipmap(GL_TEXTURE_2D);
		tex->mipmaps_outdated = false;
	}
}

const char* gl33_texture_get_debug_label(Texture *tex) {
	return tex->debug_label;
}

void gl33_texture_set_debug_label(Texture *tex, const char *label) {
	if(label) {
		log_debug("\"%s\" renamed to \"%s\"", tex->debug_label, label);
		strlcpy(tex->debug_label, label, sizeof(tex->debug_label));
	} else {
		char tmp[sizeof(tex->debug_label)];
		snprintf(tmp, sizeof(tmp), "Texture #%i", tex->gl_handle);
		log_debug("\"%s\" renamed to \"%s\"", tex->debug_label, tmp);
		strlcpy(tex->debug_label, tmp, sizeof(tex->debug_label));
	}

	glcommon_debug_object_label(GL_TEXTURE, tex->gl_handle, tex->debug_label);
}
