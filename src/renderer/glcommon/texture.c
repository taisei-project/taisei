/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "texture.h"
#include "util.h"
#include "../api.h"
#include "vtable.h"

static GLTextureFormatTuple* glcommon_find_best_pixformat_internal(GLTextureFormatTuple *formats, PixmapFormat pxfmt) {
	GLTextureFormatTuple *best = formats;

	if(best->px_fmt == pxfmt) {
		return best;
	}

	size_t ideal_channels = PIXMAP_FORMAT_LAYOUT(pxfmt);
	size_t ideal_depth = PIXMAP_FORMAT_DEPTH(pxfmt);
	bool ideal_is_float = PIXMAP_FORMAT_IS_FLOAT(pxfmt);
	bool best_is_float = PIXMAP_FORMAT_IS_FLOAT(best->px_fmt);

	for(GLTextureFormatTuple *fmt = formats + 1; fmt->px_fmt; ++fmt) {
		if(pxfmt == fmt->px_fmt) {
			return fmt;
		}

		size_t best_channels = PIXMAP_FORMAT_LAYOUT(best->px_fmt);
		size_t best_depth = PIXMAP_FORMAT_DEPTH(best->px_fmt);
		best_is_float = PIXMAP_FORMAT_IS_FLOAT(best->px_fmt);

		size_t this_channels = PIXMAP_FORMAT_LAYOUT(fmt->px_fmt);
		size_t this_depth = PIXMAP_FORMAT_DEPTH(fmt->px_fmt);
		bool this_is_float = PIXMAP_FORMAT_IS_FLOAT(fmt->px_fmt);

		if(best_channels < ideal_channels) {
			if(
				this_channels > best_channels || (
					this_channels == best_channels && (
						(this_is_float && ideal_is_float && !best_is_float) || this_depth >= ideal_depth
					)
				)
			) {
				best = fmt;
			}
		} else if(ideal_is_float && !best_is_float) {
			if(this_channels >= ideal_channels && (this_is_float || this_depth >= best_depth)) {
				best = fmt;
			}
		} else if(best_depth < ideal_depth) {
			if(this_channels >= ideal_channels && this_depth >= best_depth) {
				best = fmt;
			}
		} else if(
			this_channels >= ideal_channels &&
			this_depth >= ideal_depth &&
			this_is_float == ideal_is_float &&
			(this_channels < best_channels || this_depth < best_depth)
		) {
			best = fmt;
		}
	}

	log_debug("(%u, %u, %s) --> (%u, %u, %s)",
		PIXMAP_FORMAT_LAYOUT(pxfmt),
		PIXMAP_FORMAT_DEPTH(pxfmt),
		ideal_is_float ? "float" : "uint",
		PIXMAP_FORMAT_LAYOUT(best->px_fmt),
		PIXMAP_FORMAT_DEPTH(best->px_fmt),
		best_is_float ? "float" : "uint"
	);

	return best;
}

GLTextureFormatTuple* glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt) {
	GLTextureFormatTuple *formats = GLVT.texture_type_info(textype)->external_formats;
	return glcommon_find_best_pixformat_internal(formats, pxfmt);
}

GLenum glcommon_texture_base_format(GLenum internal_fmt) {
	switch(internal_fmt) {
		// NOTE: semi-generated code
		case GL_COMPRESSED_RED: return GL_RED;
		case GL_COMPRESSED_RED_RGTC1: return GL_RED;
		case GL_COMPRESSED_RG: return GL_RG;
		case GL_COMPRESSED_RGB: return GL_RGB;
		case GL_COMPRESSED_RGBA: return GL_RGBA;
		case GL_COMPRESSED_RG_RGTC2: return GL_RG;
		case GL_COMPRESSED_SIGNED_RED_RGTC1: return GL_RED;
		case GL_COMPRESSED_SIGNED_RG_RGTC2: return GL_RG;
		case GL_COMPRESSED_SRGB: return GL_RGB;
		case GL_COMPRESSED_SRGB_ALPHA: return GL_RGBA;
		case GL_DEPTH24_STENCIL8: return GL_DEPTH_STENCIL;
		case GL_DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL;
		case GL_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT32: return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
		case GL_R11F_G11F_B10F: return GL_RGB;
		case GL_R16: return GL_RED;
		case GL_R16F: return GL_RED;
		case GL_R16I: return GL_RED;
		case GL_R16UI: return GL_RED;
		case GL_R16_SNORM: return GL_RED;
		case GL_R32F: return GL_RED;
		case GL_R32I: return GL_RED;
		case GL_R32UI: return GL_RED;
		case GL_R3_G3_B2: return GL_RGB;
		case GL_R8: return GL_RED;
		case GL_R8I: return GL_RED;
		case GL_R8UI: return GL_RED;
		case GL_R8_SNORM: return GL_RED;
		case GL_RG16: return GL_RG;
		case GL_RG16F: return GL_RG;
		case GL_RG16I: return GL_RG;
		case GL_RG16UI: return GL_RG;
		case GL_RG16_SNORM: return GL_RG;
		case GL_RG32F: return GL_RG;
		case GL_RG32I: return GL_RG;
		case GL_RG32UI: return GL_RG;
		case GL_RG8: return GL_RG;
		case GL_RG8I: return GL_RG;
		case GL_RG8UI: return GL_RG;
		case GL_RG8_SNORM: return GL_RG;
		case GL_RGB10: return GL_RGB;
		case GL_RGB10_A2: return GL_RGBA;
		case GL_RGB10_A2UI: return GL_RGBA;
		case GL_RGB12: return GL_RGB;
		case GL_RGB16: return GL_RGB;
		case GL_RGB16F: return GL_RGB;
		case GL_RGB16I: return GL_RGB;
		case GL_RGB16UI: return GL_RGB;
		case GL_RGB16_SNORM: return GL_RGB;
		case GL_RGB32F: return GL_RGB;
		case GL_RGB32I: return GL_RGB;
		case GL_RGB32UI: return GL_RGB;
		case GL_RGB4: return GL_RGB;
		case GL_RGB5: return GL_RGB;
		case GL_RGB5_A1: return GL_RGBA;
		case GL_RGB8: return GL_RGB;
		case GL_RGB8I: return GL_RGB;
		case GL_RGB8UI: return GL_RGB;
		case GL_RGB8_SNORM: return GL_RGB;
		case GL_RGB9_E5: return GL_RGB;
		case GL_RGBA12: return GL_RGBA;
		case GL_RGBA16: return GL_RGBA;
		case GL_RGBA16F: return GL_RGBA;
		case GL_RGBA16I: return GL_RGBA;
		case GL_RGBA16UI: return GL_RGBA;
		case GL_RGBA16_SNORM: return GL_RGBA;
		case GL_RGBA2: return GL_RGBA;
		case GL_RGBA32F: return GL_RGBA;
		case GL_RGBA32I: return GL_RGBA;
		case GL_RGBA32UI: return GL_RGBA;
		case GL_RGBA4: return GL_RGBA;
		case GL_RGBA8: return GL_RGBA;
		case GL_RGBA8I: return GL_RGBA;
		case GL_RGBA8UI: return GL_RGBA;
		case GL_RGBA8_SNORM: return GL_RGBA;
		case GL_SRGB8: return GL_RGB;
		case GL_SRGB8_ALPHA8: return GL_RGBA;
	}

	UNREACHABLE;
}
