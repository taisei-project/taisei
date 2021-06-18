/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "texture.h"
#include "../api.h"
#include "opengl.h"
#include "gl33.h"
#include "../glcommon/debug.h"

static GLenum linear_to_nearest(GLenum filter) {
	switch(filter) {
		case GL_LINEAR:
			return GL_NEAREST;

		case GL_LINEAR_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;

		default:
			return GL_NONE;
	}
}

static GLuint r_filter_to_gl_filter(TextureFilterMode mode, GLTextureFormatInfo *fmt_info) {
	static GLuint map[] = {
		[TEX_FILTER_LINEAR]  = GL_LINEAR,
		[TEX_FILTER_LINEAR_MIPMAP_NEAREST]  = GL_LINEAR_MIPMAP_NEAREST,
		[TEX_FILTER_LINEAR_MIPMAP_LINEAR]  = GL_LINEAR_MIPMAP_LINEAR,
		[TEX_FILTER_NEAREST] = GL_NEAREST,
		[TEX_FILTER_NEAREST_MIPMAP_NEAREST]  = GL_NEAREST_MIPMAP_NEAREST,
		[TEX_FILTER_NEAREST_MIPMAP_LINEAR]  = GL_NEAREST_MIPMAP_LINEAR,
	};

	assert((uint)mode < sizeof(map)/sizeof(*map));
	GLenum filter = map[mode];

	if(fmt_info->flags & GLTEX_FILTERABLE) {
		return filter;
	}

	return linear_to_nearest(filter);
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

typedef struct FormatFilterFlags {
	GLTextureFormatFlags require, reject;
} FormatFilterFlags;

static GLTextureFormatInfo *pick_format(TextureType type, TextureFlags flags) {
	GLTextureFormatMatchConfig cfg = { 0 };

	cfg.intended_type = type;
	cfg.flags.desirable |= GLTEX_FILTERABLE;

	if(TEX_TYPE_IS_COMPRESSED(type)) {
		cfg.flags.required |= GLTEX_COMPRESSED;
	} else {
		cfg.flags.forbidden |= GLTEX_COMPRESSED;
		if(TEX_TYPE_IS_DEPTH(type)) {
			cfg.flags.required |= GLTEX_DEPTH_RENDERABLE;
		} else {
			cfg.flags.required |= GLTEX_COLOR_RENDERABLE | GLTEX_BLENDABLE;
		}
	}

	if(!TEX_TYPE_IS_DEPTH(type)) {
		if(flags & TEX_FLAG_SRGB) {
			cfg.flags.required |= GLTEX_SRGB;
		} else {
			cfg.flags.forbidden |= GLTEX_SRGB;
		}
	}

	switch(type) {
		case TEX_TYPE_RGBA_16_FLOAT:
		case TEX_TYPE_RGB_16_FLOAT:
		case TEX_TYPE_RG_16_FLOAT:
		case TEX_TYPE_R_16_FLOAT:
		case TEX_TYPE_RGBA_32_FLOAT:
		case TEX_TYPE_RGB_32_FLOAT:
		case TEX_TYPE_RG_32_FLOAT:
		case TEX_TYPE_R_32_FLOAT:
		case TEX_TYPE_DEPTH_16_FLOAT:
		case TEX_TYPE_DEPTH_32_FLOAT:
			cfg.flags.required |= GLTEX_FLOAT;
			break;

		default:
			cfg.flags.undesirable |= GLTEX_FLOAT;
			break;
	}

	GLTextureFormatInfo *fmt_info = glcommon_match_format(&cfg);
	return fmt_info;
}

static void gl33_texture_type_query_fmtinfo(GLTextureFormatInfo *fmt_info, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result) {
	result->optimal_pixmap_format = fmt_info->transfer_format.pixmap_format;
	result->optimal_pixmap_origin = PIXMAP_ORIGIN_BOTTOMLEFT;

	// TODO: Perhaps allow suboptimal transfer formats on non-GLES, as we did previously.
	result->supplied_pixmap_format_supported = (pxfmt == result->optimal_pixmap_format);
	result->supplied_pixmap_origin_supported = (pxorigin == result->optimal_pixmap_origin);
}

bool gl33_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result) {
	GLTextureFormatInfo *fmt_info = pick_format(type, flags);

	if(fmt_info == NULL) {
		return false;
	}

	if(result) {
		gl33_texture_type_query_fmtinfo(fmt_info, pxfmt, pxorigin, result);
	}

	return true;
}

void gl33_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) {
	if(mipmap >= tex->params.mipmaps) {
		mipmap = tex->params.mipmaps - 1;
	}

	assert(mipmap < 32);

	if(mipmap == 0) {
		if(width != NULL) {
			*width = tex->params.width;
		}

		if(height != NULL) {
			*height = tex->params.height;
		}
	} else {
		if(width != NULL) {
			*width = umax(1, tex->params.width / (1u << mipmap));
		}

		if(height != NULL) {
			*height = umax(1, tex->params.height / (1u << mipmap));
		}
	}
}

static void gl33_texture_set(Texture *tex, uint mipmap, const Pixmap *image) {
	assert(mipmap < tex->params.mipmaps);
	assert(image != NULL);

	TextureTypeQueryResult qr;
	gl33_texture_type_query_fmtinfo(tex->fmt_info, image->format, image->origin, &qr);
	assert(qr.supplied_pixmap_format_supported);
	assert(qr.supplied_pixmap_origin_supported);
	GLTextureTransferFormatInfo *xfer = &tex->fmt_info->transfer_format;

	void *image_data = image->data.untyped;

	GLuint prev_pbo = 0;

	gl33_bind_texture(tex, false, -1);
	gl33_sync_texunit(tex->binding_unit, false, true);

	if(tex->pbo) {
		prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_UNPACK);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK, tex->pbo);
		gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, image->data_size, image_data, GL_STREAM_DRAW);
		image_data = NULL;
	}

	uint width, height;
	gl33_texture_get_size(tex, mipmap, &width, &height);

	assert(width == image->width);
	assert(height == image->height);

	if(tex->fmt_info->flags & GLTEX_COMPRESSED) {
		glCompressedTexImage2D(
			GL_TEXTURE_2D,
			mipmap,
			tex->fmt_info->internal_format,
			width,
			height,
			0,
			image->data_size,
			image_data
		);
	} else {
		glTexImage2D(
			GL_TEXTURE_2D,
			mipmap,
			tex->fmt_info->internal_format,
			width,
			height,
			0,
			xfer->gl_format,
			xfer->gl_type,
			image_data
		);
	}

	if(tex->pbo) {
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_UNPACK, prev_pbo);
	}

	tex->mipmaps_outdated = true;
}

static void apply_swizzle(GLenum param, char val) {
	GLenum swizzle_val, default_val;

	switch(param) {
		case GL_TEXTURE_SWIZZLE_R: default_val = GL_RED;   break;
		case GL_TEXTURE_SWIZZLE_G: default_val = GL_GREEN; break;
		case GL_TEXTURE_SWIZZLE_B: default_val = GL_BLUE;  break;
		case GL_TEXTURE_SWIZZLE_A: default_val = GL_ALPHA; break;
		default: UNREACHABLE;
	}

	switch(val) {
		case 'r': swizzle_val = GL_RED;      break;
		case 'g': swizzle_val = GL_GREEN;    break;
		case 'b': swizzle_val = GL_BLUE;     break;
		case 'a': swizzle_val = GL_ALPHA;    break;
		case '0': swizzle_val = GL_ZERO;     break;
		case '1': swizzle_val = GL_ONE;      break;
		case  0 : swizzle_val = default_val; break;
		default: UNREACHABLE;
	}

	if(glext.texture_swizzle) {
		assert(r_supports(RFEAT_TEXTURE_SWIZZLE));
		glTexParameteri(GL_TEXTURE_2D, param, swizzle_val);
	} else if(default_val != swizzle_val) {
		log_warn("Texture swizzle mask differs from default; not supported in this OpenGL version!");
	}
}

static void apply_swizzle_mask(SwizzleMask *mask) {
	apply_swizzle(GL_TEXTURE_SWIZZLE_R, mask->r);
	apply_swizzle(GL_TEXTURE_SWIZZLE_G, mask->g);
	apply_swizzle(GL_TEXTURE_SWIZZLE_B, mask->b);
	apply_swizzle(GL_TEXTURE_SWIZZLE_A, mask->a);
}

Texture* gl33_texture_create(const TextureParams *params) {
	Texture *tex = calloc(1, sizeof(Texture));
	memcpy(&tex->params, params, sizeof(*params));
	TextureParams *p = &tex->params;

	uint max_mipmaps = r_texture_util_max_num_miplevels(p->width, p->height);
	assert(max_mipmaps > 0);

	if(p->mipmaps == 0) {
		if(p->mipmap_mode == TEX_MIPMAP_AUTO) {
			p->mipmaps = TEX_MIPMAPS_MAX;
			if(p->flags & TEX_FLAG_SRGB) {
				log_fatal_if_debug("FIXME: sRGB textures may not support automatic mipmap generation; please write a fallback!");
			}
		} else {
			p->mipmaps = 1;
		}
	}

	if(p->mipmaps == TEX_MIPMAPS_MAX || p->mipmaps > max_mipmaps) {
		p->mipmaps = max_mipmaps;
	}

	bool partial_mipmaps_ok = r_supports(RFEAT_PARTIAL_MIPMAPS);

	if(!partial_mipmaps_ok && p->mipmap_mode == TEX_MIPMAP_MANUAL && p->mipmaps > 1 && p->mipmaps < max_mipmaps) {
		log_fatal_if_debug("Partial mipmaps not supported in this OpenGL version");
	}

	if(p->anisotropy == 0) {
		p->anisotropy = TEX_ANISOTROPY_DEFAULT;
	}

	glGenTextures(1, &tex->gl_handle);
	snprintf(tex->debug_label, sizeof(tex->debug_label), "Texture #%i", tex->gl_handle);
	gl33_bind_texture(tex, false, -1);
	gl33_sync_texunit(tex->binding_unit, false, true);

	tex->fmt_info = pick_format(params->type, params->flags);

	if(!tex->fmt_info) {
		log_fatal("Failed to match GL format for texture type 0x%04x with flags 0x%04x", params->type, params->flags);
	}

	GLTextureTransferFormatInfo *xfer = &tex->fmt_info->transfer_format;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(p->wrap.s));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(p->wrap.t));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(p->filter.min, tex->fmt_info));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(p->filter.mag, tex->fmt_info));

	apply_swizzle_mask(&tex->params.swizzle);

	if(partial_mipmaps_ok) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, p->mipmaps - 1);
	}

	if(glext.texture_filter_anisotropic) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, p->anisotropy);
	}

	if((p->flags & TEX_FLAG_STREAM) && glext.pixel_buffer_object) {
		glGenBuffers(1, &tex->pbo);
	}

	for(uint i = 0; i < p->mipmaps; ++i) {
		uint width, height;
		gl33_texture_get_size(tex, i, &width, &height);

		if(tex->fmt_info->flags & GLTEX_COMPRESSED) {
			// XXX: can't pre-allocate this without ARB_texture_storage or equivalent
		} else {
			glTexImage2D(
				GL_TEXTURE_2D,
				i,
				tex->fmt_info->internal_format,
				width,
				height,
				0,
				xfer->gl_format,
				xfer->gl_type,
				NULL
			);
		}
	}

	return tex;
}

void gl33_texture_get_params(Texture *tex, TextureParams *params) {
	memcpy(params, &tex->params, sizeof(*params));
}

void gl33_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	if(tex->params.filter.min != fmin) {
		gl33_bind_texture(tex, false, -1);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.filter.min = fmin;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, r_filter_to_gl_filter(fmin, tex->fmt_info));
	}

	if(tex->params.filter.mag != fmag) {
		gl33_bind_texture(tex, false, -1);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.filter.mag = fmag;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, r_filter_to_gl_filter(fmag, tex->fmt_info));
	}
}

void gl33_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	if(tex->params.wrap.s != ws) {
		gl33_bind_texture(tex, false, -1);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.wrap.s = ws;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, r_wrap_to_gl_wrap(ws));
	}

	if(tex->params.wrap.t != wt) {
		gl33_bind_texture(tex, false, -1);
		gl33_sync_texunit(tex->binding_unit, false, true);
		tex->params.wrap.t = wt;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, r_wrap_to_gl_wrap(wt));
	}
}

void gl33_texture_invalidate(Texture *tex) {
	if(tex->fmt_info->flags & GLTEX_COMPRESSED) {
		log_debug("TODO/FIXME: invalidate not implemented for compressed textures");
		return;
	}

	gl33_bind_texture(tex, false, -1);
	gl33_sync_texunit(tex->binding_unit, false, true);

	for(uint i = 0; i < tex->params.mipmaps; ++i) {
		uint width, height;
		gl33_texture_get_size(tex, i, &width, &height);

		glTexImage2D(
			GL_TEXTURE_2D,
			i,
			tex->fmt_info->internal_format,
			width,
			height,
			0,
			tex->fmt_info->transfer_format.gl_format,
			tex->fmt_info->transfer_format.gl_type,
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

	gl33_bind_texture(tex, false, -1);
	gl33_sync_texunit(tex->binding_unit, false, true);

	TextureTypeQueryResult qr;
	gl33_texture_type_query_fmtinfo(tex->fmt_info, image->format, image->origin, &qr);
	assert(qr.supplied_pixmap_format_supported);
	assert(qr.supplied_pixmap_origin_supported);
	GLTextureTransferFormatInfo *xfer = &tex->fmt_info->transfer_format;

	if(tex->fmt_info->flags & GLTEX_COMPRESSED) {
		glCompressedTexSubImage2D(
			GL_TEXTURE_2D, mipmap,
			x, tex->params.height - y - image->height, image->width, image->height,
			tex->fmt_info->internal_format,
			image->data_size,
			image->data.untyped
		);
	} else {
		glTexSubImage2D(
			GL_TEXTURE_2D, mipmap,
			x, tex->params.height - y - image->height, image->width, image->height,
			xfer->gl_format,
			xfer->gl_type,
			image->data.untyped
		);
	}

	tex->mipmaps_outdated = true;
}

void gl44_texture_clear(Texture *tex, const Color *clr) {
#ifdef STATIC_GLES3
	UNREACHABLE;
#else
	for(int i = 0; i < tex->params.mipmaps; ++i) {
		glClearTexImage(tex->gl_handle, i, GL_RGBA, GL_FLOAT, &clr->r);
	}
#endif
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
		// log_debug("Generating mipmaps for %s (%u)", tex->debug_label, tex->gl_handle);

		gl33_bind_texture(tex, false, -1);
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
