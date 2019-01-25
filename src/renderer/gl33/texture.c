/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "texture.h"
#include "../api.h"
#include "opengl.h"
#include "gl33.h"
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
	static GLTextureFormatTuple color_formats[] = {
		{ GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8     },
		{ GL_RED,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16    },
		{ GL_RED,  GL_UNSIGNED_INT,   PIXMAP_FORMAT_R32    },

		{ GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8    },
		{ GL_RG,   GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RG16   },
		{ GL_RG,   GL_UNSIGNED_INT,   PIXMAP_FORMAT_RG32   },

		{ GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8   },
		{ GL_RGB,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGB16  },
		{ GL_RGB,  GL_UNSIGNED_INT,   PIXMAP_FORMAT_RGB32  },

		{ GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8  },
		{ GL_RGBA, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGBA16 },
		{ GL_RGBA, GL_UNSIGNED_INT,   PIXMAP_FORMAT_RGBA32 },

		{ 0 }
	};

	// XXX: I'm not sure about this. Perhaps it's better to not support depth texture uploading at all?
	static GLTextureFormatTuple depth_formats[] = {
		{ GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8 },
		{ GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 },
		{ GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,   PIXMAP_FORMAT_R32 },

		{ 0 }
	};

	static GLTextureTypeInfo map[] = {
		[TEX_TYPE_R_8]            = { GL_R8,     color_formats, { GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8    } },
		[TEX_TYPE_RG_8]           = { GL_RG8,    color_formats, { GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8   } },
		[TEX_TYPE_RGB_8]          = { GL_RGB8,   color_formats, { GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8  } },
		[TEX_TYPE_RGBA_8]         = { GL_RGBA8,  color_formats, { GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8 } },
		[TEX_TYPE_DEPTH_8]        = { GL_DEPTH_COMPONENT16, depth_formats, { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 } },

		[TEX_TYPE_R_16]           = { GL_R16,    color_formats, { GL_RED,  GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_R16    } },
		[TEX_TYPE_RG_16]          = { GL_RG16,   color_formats, { GL_RG,   GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RG16   } },
		[TEX_TYPE_RGB_16]         = { GL_RGB16,  color_formats, { GL_RGB,  GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RGB16  } },
		[TEX_TYPE_RGBA_16]        = { GL_RGBA16, color_formats, { GL_RGBA, GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RGBA16 } },
		[TEX_TYPE_DEPTH_16]       = { GL_DEPTH_COMPONENT16, depth_formats, { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 } },

		[TEX_TYPE_R_32_FLOAT]     = { GL_R32F,    color_formats, { GL_RED,  GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_R16    } },
		[TEX_TYPE_RG_32_FLOAT]    = { GL_RG32F,   color_formats, { GL_RG,   GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RG16   } },
		[TEX_TYPE_RGB_32_FLOAT]   = { GL_RGB32F,  color_formats, { GL_RGB,  GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RGB16  } },
		[TEX_TYPE_RGBA_32_FLOAT]  = { GL_RGBA32F, color_formats, { GL_RGBA, GL_UNSIGNED_SHORT,  PIXMAP_FORMAT_RGBA16 } },
		[TEX_TYPE_DEPTH_32_FLOAT] = { GL_DEPTH_COMPONENT32, depth_formats, { GL_DEPTH_COMPONENT32F, GL_FLOAT, PIXMAP_FORMAT_R32 } },
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

static GLTextureFormatTuple* prepare_pixmap(Texture *tex, const Pixmap *px_in, Pixmap *px_out) {
	GLTextureFormatTuple *fmt = glcommon_find_best_pixformat(tex->params.type, px_in->format);
	pixmap_convert_alloc(px_in, px_out, fmt->px_fmt);
	pixmap_flip_to_origin_inplace(px_out, PIXMAP_ORIGIN_BOTTOMLEFT);
	return fmt;
}

static void gl33_texture_set(Texture *tex, uint mipmap, const Pixmap *image) {
	assert(mipmap < tex->params.mipmaps);
	assert(image != NULL);

	Pixmap pix;
	GLTextureFormatTuple *fmt = prepare_pixmap(tex, image, &pix);
	void *image_data = pix.data.untyped;

	GLuint prev_pbo = 0;

	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);

	if(tex->pbo) {
		prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_UNPACK);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK, tex->pbo);
		gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, pixmap_data_size(&pix), image_data, GL_STREAM_DRAW);
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
		fmt->gl_fmt,
		fmt->gl_type,
		image_data
	);

	free(pix.data.untyped);

	if(tex->pbo) {
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK, prev_pbo);
	}

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

	if(!glext.version.is_es || GLES_ATLEAST(3, 0)) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, p->mipmaps - 1);
	}

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
			tex->type_info->primary_external_format.gl_fmt,
			tex->type_info->primary_external_format.gl_type,
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
	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);

	for(uint i = 0; i < tex->params.mipmaps; ++i) {
		uint width, height;
		gl33_texture_get_size(tex, i, &width, &height);

		glTexImage2D(
			GL_TEXTURE_2D,
			i,
			tex->type_info->internal_fmt,
			width,
			height,
			0,
			tex->type_info->primary_external_format.gl_fmt,
			tex->type_info->primary_external_format.gl_type,
			NULL
		);
	}
}

void gl33_texture_fill(Texture *tex, uint mipmap, const Pixmap *image) {
	assert(mipmap == 0 || tex->params.mipmap_mode != TEX_MIPMAP_AUTO);
	gl33_texture_set(tex, mipmap, image);
}

void gl33_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, const Pixmap *image) {
	assert(mipmap == 0 || tex->params.mipmap_mode != TEX_MIPMAP_AUTO);

	gl33_bind_texture(tex, false);
	gl33_sync_texunit(tex->binding_unit, false, true);

	Pixmap pix;
	GLTextureFormatTuple *fmt = prepare_pixmap(tex, image, &pix);

	glTexSubImage2D(
		GL_TEXTURE_2D, mipmap,
		x, y, pix.width, pix.height,
		fmt->gl_fmt,
		fmt->gl_type,
		pix.data.untyped
	);

	free(pix.data.untyped);
	tex->mipmaps_outdated = true;
}

void gl44_texture_clear(Texture *tex, const Color *clr) {
	for(int i = 0; i < tex->params.mipmaps; ++i) {
		glClearTexImage(tex->gl_handle, i, GL_RGBA, GL_FLOAT, &clr->r);
	}
}

void gl33_texture_clear(Texture *tex, const Color *clr) {
	// TODO: maybe find a more efficient method
	Framebuffer *temp_fb = r_framebuffer_create();
	r_framebuffer_attach(temp_fb, tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_clear(temp_fb, CLEAR_COLOR, clr, 1);
	r_framebuffer_destroy(temp_fb);
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
	glcommon_set_debug_label(tex->debug_label, "Texture", GL_TEXTURE, tex->gl_handle, label);
}
