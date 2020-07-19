/*
 * This software is licensed under the terms of the MIT License.
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

static GLTextureFormatTuple *glcommon_find_best_pixformat_internal(GLTextureFormatTuple *formats, PixmapFormat pxfmt) {
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

GLTextureFormatTuple *glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt) {
	GLTextureFormatTuple *formats = GLVT.texture_type_info(textype)->external_formats;
	return glcommon_find_best_pixformat_internal(formats, pxfmt);
}

GLenum glcommon_texture_base_format(GLenum internal_fmt) {
	switch(internal_fmt) {
		// NOTE: semi-generated code

		#ifndef STATIC_GLES3
		case GL_R3_G3_B2: return GL_RGB;
		case GL_RGB12: return GL_RGB;
		case GL_RGB4: return GL_RGB;
		case GL_RGB5: return GL_RGB;
		case GL_RGBA12: return GL_RGBA;
		case GL_RGBA2: return GL_RGBA;
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
		#endif

		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR: return GL_RGBA;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return GL_RGB;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return GL_RGBA;
		case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB: return GL_RGBA;
		case GL_ETC1_RGB8_OES: return GL_RGB;
		case GL_COMPRESSED_RGBA8_ETC2_EAC: return GL_RGBA;
		case GL_COMPRESSED_R11_EAC: return GL_RED;
		case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG: return GL_RGB;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG: return GL_RGBA;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG: return GL_RGBA;
		case GL_ATC_RGB_AMD: return GL_RGB;
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD: return GL_RGBA;
		case GL_COMPRESSED_RGB_FXT1_3DFX: return GL_RGB;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR: return GL_RGBA;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return GL_RGBA;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return GL_RGBA;
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB: return GL_RGBA;
		case GL_ETC1_SRGB8_NV: return GL_RGB;
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return GL_RGBA;
		case GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT: return GL_RGB;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT: return GL_RGBA;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG: return GL_RGBA;

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
		case GL_RGB16: return GL_RGB;
		case GL_RGB16F: return GL_RGB;
		case GL_RGB16I: return GL_RGB;
		case GL_RGB16UI: return GL_RGB;
		case GL_RGB16_SNORM: return GL_RGB;
		case GL_RGB32F: return GL_RGB;
		case GL_RGB32I: return GL_RGB;
		case GL_RGB32UI: return GL_RGB;
		case GL_RGB5_A1: return GL_RGBA;
		case GL_RGB8: return GL_RGB;
		case GL_RGB8I: return GL_RGB;
		case GL_RGB8UI: return GL_RGB;
		case GL_RGB8_SNORM: return GL_RGB;
		case GL_RGB9_E5: return GL_RGB;
		case GL_RGBA16: return GL_RGBA;
		case GL_RGBA16F: return GL_RGBA;
		case GL_RGBA16I: return GL_RGBA;
		case GL_RGBA16UI: return GL_RGBA;
		case GL_RGBA16_SNORM: return GL_RGBA;
		case GL_RGBA32F: return GL_RGBA;
		case GL_RGBA32I: return GL_RGBA;
		case GL_RGBA32UI: return GL_RGBA;
		case GL_RGBA4: return GL_RGBA;
		case GL_RGBA8: return GL_RGBA;
		case GL_RGBA8I: return GL_RGBA;
		case GL_RGBA8UI: return GL_RGBA;
		case GL_RGBA8_SNORM: return GL_RGBA;
		case GL_SR8_EXT: return GL_RED;
		case GL_SRG8_EXT: return GL_RG;
		case GL_SRGB8: return GL_RGB;
		case GL_SRGB8_ALPHA8: return GL_RGBA;

		// GLES2 may use these as internal formats
		case GL_RED: return GL_RED;
		case GL_RG: return GL_RG;
		case GL_RGB: return GL_RGB;
		case GL_RGBA: return GL_RGBA;
		case GL_SRGB: return GL_RGB;
		case GL_SRGB_ALPHA: return GL_RGBA;
		case GL_DEPTH_COMPONENT: return GL_DEPTH_COMPONENT;
	}

	UNREACHABLE;
}

GLenum glcommon_compression_to_gl_format(PixmapCompression cfmt) {
	GLenum r = GL_NONE;

	switch(cfmt) {
		case PIXMAP_COMPRESSION_ASTC_4x4_RGBA:
			if(glext.tex_format.astc) {
				r = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
			}
			break;

		case PIXMAP_COMPRESSION_BC1_RGB:
			if(glext.tex_format.s3tc_dx1) {
				r = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_BC3_RGBA:
			if(glext.tex_format.s3tc_dx5) {
				r = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_BC4_R:
			if(glext.tex_format.rgtc) {
				r = GL_COMPRESSED_RED_RGTC1;
			}
			break;

		case PIXMAP_COMPRESSION_BC5_RG:
			if(glext.tex_format.rgtc) {
				r = GL_COMPRESSED_RG_RGTC2;
			}
			break;

		case PIXMAP_COMPRESSION_BC7_RGBA:
			if(glext.tex_format.bptc) {
				r = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
			}
			break;

		case PIXMAP_COMPRESSION_ETC1_RGB:
			if(glext.tex_format.etc1) {
				r = GL_ETC1_RGB8_OES;
				break;
			}
			// NOTE: this format is forwards-compatible with ETC2
			// fallthrough

		case PIXMAP_COMPRESSION_ETC2_RGBA:
			if(glext.tex_format.etc2_eac) {
				r = GL_COMPRESSED_RGBA8_ETC2_EAC;
			}
			break;

		case PIXMAP_COMPRESSION_ETC2_EAC_R11:
			if(glext.tex_format.etc2_eac) {
				r = GL_COMPRESSED_R11_EAC;
			}
			break;

		case PIXMAP_COMPRESSION_ETC2_EAC_RG11:
			if(glext.tex_format.etc2_eac) {
				r = GL_COMPRESSED_RG11_EAC;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC1_4_RGB:
			if(glext.tex_format.pvrtc) {
				r = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC1_4_RGBA:
			if(glext.tex_format.pvrtc) {
				r = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC2_4_RGB:
		case PIXMAP_COMPRESSION_PVRTC2_4_RGBA:
			if(glext.tex_format.pvrtc2) {
				r = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
			}
			break;

		case PIXMAP_COMPRESSION_ATC_RGB:
			if(glext.tex_format.atc) {
				r = GL_ATC_RGB_AMD;
			}
			break;

		case PIXMAP_COMPRESSION_ATC_RGBA:
			if(glext.tex_format.atc) {
				r = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
			}
			break;

		case PIXMAP_COMPRESSION_FXT1_RGB:
			if(glext.tex_format.fxt1) {
				r = GL_COMPRESSED_RGB_FXT1_3DFX;
			}
			break;

		default: UNREACHABLE;
	}

	return r;
}

GLenum glcommon_compression_to_gl_format_srgb(PixmapCompression cfmt) {
	GLenum r = GL_NONE;

	switch(cfmt) {
		case PIXMAP_COMPRESSION_ASTC_4x4_RGBA:
			if(glext.tex_format.astc) {
				r = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
			}
			break;

		case PIXMAP_COMPRESSION_BC1_RGB:
			if(glext.tex_format.s3tc_dx1 && glext.tex_format.s3tc_srgb) {
				r = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_BC3_RGBA:
			if(glext.tex_format.s3tc_dx5 && glext.tex_format.s3tc_srgb) {
				r = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_BC7_RGBA:
			if(glext.tex_format.bptc) {
				r = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
			}
			break;

		case PIXMAP_COMPRESSION_ETC1_RGB:
			if(glext.tex_format.etc1_srgb) {
				r = GL_ETC1_SRGB8_NV;
				break;
			}
			// NOTE: this format is forwards-compatible with ETC2
			// fallthrough

		case PIXMAP_COMPRESSION_ETC2_RGBA:
			if(glext.tex_format.etc2_eac_srgb) {
				r = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC1_4_RGB:
			if(glext.tex_format.pvrtc && glext.tex_format.pvrtc_srgb) {
				r = GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC1_4_RGBA:
			if(glext.tex_format.pvrtc && glext.tex_format.pvrtc_srgb) {
				r = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT;
			}
			break;

		case PIXMAP_COMPRESSION_PVRTC2_4_RGB:
		case PIXMAP_COMPRESSION_PVRTC2_4_RGBA:
			if(glext.tex_format.pvrtc2 && glext.tex_format.pvrtc_srgb) {
				r = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG;
			}
			break;

		case PIXMAP_COMPRESSION_BC4_R:
		case PIXMAP_COMPRESSION_BC5_RG:
		case PIXMAP_COMPRESSION_ETC2_EAC_R11:
		case PIXMAP_COMPRESSION_ETC2_EAC_RG11:
		case PIXMAP_COMPRESSION_ATC_RGB:
		case PIXMAP_COMPRESSION_ATC_RGBA:
		case PIXMAP_COMPRESSION_FXT1_RGB:
			break;

		default: UNREACHABLE;
	}

	return r;
}

GLenum glcommon_uncompressed_format_to_srgb_format(GLenum format) {
	switch(format) {
		case GL_RED:
		case GL_R8:
			return glext.tex_format.r8_srgb ? GL_SR8_EXT : GL_NONE;

		case GL_RG:
		case GL_RG8:
			return glext.tex_format.rg8_srgb ? GL_SRG8_EXT : GL_NONE;

		case GL_RGB:
		case GL_RGB8:
			return glext.tex_format.rgb8_rgba8_srgb ? GL_SRGB8 : GL_NONE;

		case GL_RGBA:
		case GL_RGBA8:
			return glext.tex_format.rgb8_rgba8_srgb ? GL_SRGB8_ALPHA8 : GL_NONE;

		default:
			return GL_NONE;
	}
}
