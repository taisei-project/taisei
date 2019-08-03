/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "texture.h"
#include "../glcommon/opengl.h"

#define FMT_R8      { GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8      }
#define FMT_R16     { GL_RED,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16     }
#define FMT_R32F    { GL_RED,  GL_FLOAT,          PIXMAP_FORMAT_R32F    }

#define FMT_RG8     { GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8     }
#define FMT_RG16    { GL_RG,   GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RG16    }
#define FMT_RG32F   { GL_RG,   GL_FLOAT,          PIXMAP_FORMAT_RG32F   }

#define FMT_RGB8    { GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8    }
#define FMT_RGB16   { GL_RGB,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGB16   }
#define FMT_RGB32F  { GL_RGB,  GL_FLOAT,          PIXMAP_FORMAT_RGB32F  }

#define FMT_RGBA8   { GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8   }
#define FMT_RGBA16  { GL_RGBA, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGBA16  }
#define FMT_RGBA32F { GL_RGBA, GL_FLOAT,          PIXMAP_FORMAT_RGBA32F }

#define FMT_DEPTH   { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 }

#define FMTSTRUCT(fmt) ((GLTextureFormatTuple) fmt)
#define FMTLIST(...) ((GLTextureFormatTuple[]) {__VA_ARGS__, { 0 } })
#define MAKEFMT(sized, externdef) { sized, FMTLIST(externdef), externdef }

static GLTextureTypeInfo gles_texformats[] = {
	[TEX_TYPE_R_8]            = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RG_8]           = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGB_8]          = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGBA_8]         = MAKEFMT(GL_RGBA8, FMT_RGBA8),

	[TEX_TYPE_R_16]           = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RG_16]          = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGB_16]         = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGBA_16]        = MAKEFMT(GL_RGBA8, FMT_RGBA8),

	[TEX_TYPE_R_32_FLOAT]     = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RG_32_FLOAT]    = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGB_32_FLOAT]   = MAKEFMT(GL_RGB8,  FMT_RGB8),
	[TEX_TYPE_RGBA_32_FLOAT]  = MAKEFMT(GL_RGBA8, FMT_RGBA8),

	// WARNING: ANGLE bug(?): texture binding fails if a sized format is used here.
	[TEX_TYPE_DEPTH_8]        = MAKEFMT(GL_DEPTH_COMPONENT16, FMT_DEPTH),
	[TEX_TYPE_DEPTH_16]       = MAKEFMT(GL_DEPTH_COMPONENT16, FMT_DEPTH),
	[TEX_TYPE_DEPTH_32_FLOAT] = MAKEFMT(GL_DEPTH_COMPONENT16, FMT_DEPTH),
};

static inline void set_format(TextureType t, GLenum internal, GLTextureFormatTuple external) {
	gles_texformats[t].internal_fmt = internal;
	gles_texformats[t].primary_external_format = external;
	gles_texformats[t].external_formats[0] = external;
}

void gles_init_texformats_table(void) {
	bool is_gles3 = GLES_ATLEAST(3, 0);
	bool is_angle = glext.version.is_ANGLE;
	bool have_rg = glext.texture_rg;
	bool have_16bit = glext.texture_norm16;
	bool have_renderable_float = glext.color_buffer_float;
	bool have_float16 = /* glext.texture_half_float_linear && */ have_renderable_float;
	bool have_float32 = /* glext.texture_float_linear && */ have_renderable_float && glext.float_blend;

	// XXX: Workaround for an ANGLE bug
	if(is_angle) {
		set_format(TEX_TYPE_DEPTH_8, GL_DEPTH_COMPONENT, FMTSTRUCT(FMT_DEPTH));
		set_format(TEX_TYPE_DEPTH_16, GL_DEPTH_COMPONENT, FMTSTRUCT(FMT_DEPTH));
		set_format(TEX_TYPE_DEPTH_32_FLOAT, GL_DEPTH_COMPONENT, FMTSTRUCT(FMT_DEPTH));
	}

	// WARNING: GL_RGB* is generally not color-renderable, except for GL_RGB8
	// So use GL_RGBA instead, where appropriate.

	if(have_rg) {
		set_format(TEX_TYPE_R_8, GL_R8, FMTSTRUCT(FMT_R8));
		set_format(TEX_TYPE_RG_8, GL_RG8, FMTSTRUCT(FMT_RG8));

		if(have_16bit) {
			set_format(TEX_TYPE_R_16, GL_R16, FMTSTRUCT(FMT_R16));
			set_format(TEX_TYPE_RG_16, GL_RG16, FMTSTRUCT(FMT_RG16));
		} else {
			set_format(TEX_TYPE_R_16, GL_R8, FMTSTRUCT(FMT_R8));
			set_format(TEX_TYPE_RG_16, GL_RG8, FMTSTRUCT(FMT_RG8));
		}

		if(have_float32 || have_float16) {
			set_format(TEX_TYPE_R_32_FLOAT, have_float32 ? GL_R32F : GL_R16F, FMTSTRUCT(FMT_R32F));
			set_format(TEX_TYPE_RG_32_FLOAT, have_float32 ? GL_RG32F : GL_RG16F, FMTSTRUCT(FMT_RG32F));
		} else {
			gles_texformats[TEX_TYPE_R_32_FLOAT] = gles_texformats[TEX_TYPE_R_16];
			gles_texformats[TEX_TYPE_RG_32_FLOAT] = gles_texformats[TEX_TYPE_RG_16];
		}
	} else {
		if(have_16bit) {
			set_format(TEX_TYPE_R_16, GL_RGBA16, FMTSTRUCT(FMT_RGBA16));
			set_format(TEX_TYPE_RG_16, GL_RGBA16, FMTSTRUCT(FMT_RGBA16));
		}

		if(have_float32 || have_float16) {
			set_format(TEX_TYPE_R_32_FLOAT, have_float32 ? GL_RGBA32F : GL_RGBA16F, FMTSTRUCT(FMT_RGBA32F));
			set_format(TEX_TYPE_RG_32_FLOAT, have_float32 ? GL_RGBA32F : GL_RGBA16F, FMTSTRUCT(FMT_RGBA32F));
		} else {
			gles_texformats[TEX_TYPE_R_32_FLOAT] = gles_texformats[TEX_TYPE_R_16];
			gles_texformats[TEX_TYPE_RG_32_FLOAT] = gles_texformats[TEX_TYPE_RG_16];
		}
	}

	if(have_16bit) {
		set_format(TEX_TYPE_RGB_16, GL_RGBA16, FMTSTRUCT(FMT_RGBA16));
		set_format(TEX_TYPE_RGBA_16, GL_RGBA16, FMTSTRUCT(FMT_RGBA16));
	}

	if(have_float32 || have_float16) {
		set_format(TEX_TYPE_RGB_32_FLOAT, have_float32 ? GL_RGBA32F : GL_RGBA16F, FMTSTRUCT(FMT_RGBA32F));
		set_format(TEX_TYPE_RGBA_32_FLOAT, have_float32 ? GL_RGBA32F : GL_RGBA16F, FMTSTRUCT(FMT_RGBA32F));
	} else {
		gles_texformats[TEX_TYPE_RGB_32_FLOAT] = gles_texformats[TEX_TYPE_RGBA_16];
		gles_texformats[TEX_TYPE_RGBA_32_FLOAT] = gles_texformats[TEX_TYPE_RGBA_16];
	}

	for(uint i = 0; i < sizeof(gles_texformats)/sizeof(*gles_texformats); ++i) {
		gles_texformats[i].external_formats[0] = gles_texformats[i].primary_external_format;

		if(!is_gles3) {
			gles_texformats[i].internal_fmt = glcommon_texture_base_format(gles_texformats[i].internal_fmt);
			assert(gles_texformats[i].internal_fmt == gles_texformats[i].primary_external_format.gl_fmt);
		}
	}
}

GLTextureTypeInfo* gles_texture_type_info(TextureType type) {
	assert((uint)type < sizeof(gles_texformats)/sizeof(*gles_texformats));
	return gles_texformats + type;
}

GLTexFormatCapabilities gles_texture_format_caps(GLenum internal_fmt) {
	GLTexFormatCapabilities caps = 0;

	/*
	 * Some formats we don't care about were omitted here
	 */

	switch(internal_fmt) {
		case GL_R8:
		case GL_RG8:
		case GL_RGB8:
		case GL_RGBA8:
			caps |= GLTEX_COLOR_RENDERABLE | GLTEX_FILTERABLE;
			break;

		case GL_R16:
		case GL_RG16:
		case GL_RGBA16:
			if(glext.texture_norm16) {
				caps |= GLTEX_COLOR_RENDERABLE | GLTEX_FILTERABLE;
			}
			break;

		case GL_RGB16:
			if(glext.texture_norm16) {
				caps |= GLTEX_FILTERABLE;
			}
			break;

		case GL_R16F:
		case GL_RG16F:
		case GL_RGBA16F:
			if(glext.color_buffer_float) {
				caps |= GLTEX_COLOR_RENDERABLE;
			}
			// fallthrough
		case GL_RGB16F:
			if(glext.texture_half_float_linear) {
				caps |= GLTEX_FILTERABLE;
			}
			break;

		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
			if(glext.float_blend) {
				if(glext.color_buffer_float && internal_fmt != GL_RGB32F) {
					caps |= GLTEX_COLOR_RENDERABLE;
				}

				if(glext.texture_float_linear) {
					caps |= GLTEX_FILTERABLE;
				}
			}
			break;

		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
			// FIXME: Is there an extension that makes it filterable?
			// It seems to work on Mesa and ANGLE at least.
			caps |= GLTEX_DEPTH_RENDERABLE;
			break;

		case GL_DEPTH_COMPONENT32F:
			caps |= GLTEX_DEPTH_RENDERABLE | GLTEX_FILTERABLE;
			break;
	}

	return caps;
}
