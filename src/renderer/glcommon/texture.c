/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "texture.h"
#include "vtable.h"
#include "../api.h"

#include "util.h"
#include "util/strbuf.h"

#define XFER_R8      { GL_RED,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_R8      }
#define XFER_R16     { GL_RED,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16     }
#define XFER_R32F    { GL_RED,  GL_FLOAT,          PIXMAP_FORMAT_R32F    }

#define XFER_RG8     { GL_RG,   GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RG8     }
#define XFER_RG16    { GL_RG,   GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RG16    }
#define XFER_RG32F   { GL_RG,   GL_FLOAT,          PIXMAP_FORMAT_RG32F   }

#define XFER_RGB8    { GL_RGB,  GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGB8    }
#define XFER_RGB16   { GL_RGB,  GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGB16   }
#define XFER_RGB32F  { GL_RGB,  GL_FLOAT,          PIXMAP_FORMAT_RGB32F  }

#define XFER_RGBA8   { GL_RGBA, GL_UNSIGNED_BYTE,  PIXMAP_FORMAT_RGBA8   }
#define XFER_RGBA16  { GL_RGBA, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_RGBA16  }
#define XFER_RGBA32F { GL_RGBA, GL_FLOAT,          PIXMAP_FORMAT_RGBA32F }

#define XFER_DEPTH16  { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, PIXMAP_FORMAT_R16 }
#define XFER_DEPTH24  { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, PIXMAP_FORMAT_R32 }
#define XFER_DEPTH32F { GL_DEPTH_COMPONENT, GL_FLOAT, PIXMAP_FORMAT_R32F }

#define XFER_COMPRESSED(comp) { GL_NONE, GL_NONE, PIXMAP_FORMAT_##comp }

static DYNAMIC_ARRAY(GLTextureFormatInfo) g_formats;

static void dump_tex_flags(StringBuffer *buf, GLTextureFormatFlags flags) {
	const char *prefix = "";

	#define HANDLE_FLAG(flag) \
		if(flags & flag) { \
			strbuf_printf(buf, "%s" #flag, prefix); \
			flags &= ~flag; \
			prefix = " | "; \
		}

	HANDLE_FLAG(GLTEX_COLOR_RENDERABLE)
	HANDLE_FLAG(GLTEX_DEPTH_RENDERABLE)
	HANDLE_FLAG(GLTEX_FILTERABLE)
	HANDLE_FLAG(GLTEX_BLENDABLE)
	HANDLE_FLAG(GLTEX_SRGB)
	HANDLE_FLAG(GLTEX_COMPRESSED)

	if(flags) {
		strbuf_printf(buf, "%s0x%04x", prefix, flags);
	}
}

static void dump_fmt_info(StringBuffer *buf, const GLTextureFormatInfo *fmt_info) {
	strbuf_printf(buf, "(GLTextureFormatInfo) { ");
		strbuf_printf(buf, ".intended_type_mapping = %s, ", r_texture_type_name(fmt_info->intended_type_mapping));
		strbuf_printf(buf, ".base_format = 0x%04x, ", fmt_info->base_format);
		strbuf_printf(buf, ".internal_format = 0x%04x, ", fmt_info->internal_format);
		strbuf_printf(buf, ".transfer_format = { ");
			strbuf_printf(buf, ".gl_format = 0x%04x, ", fmt_info->transfer_format.gl_format);
			strbuf_printf(buf, ".gl_type = 0x%04x, ", fmt_info->transfer_format.gl_type);
			{
				const char *n = pixmap_format_name(fmt_info->transfer_format.pixmap_format);
				if(n) {
					strbuf_printf(buf, ".pixmap_format = %s, ", n);
				} else {
					strbuf_printf(buf, ".pixmap_format = 0x%04x, ", fmt_info->transfer_format.pixmap_format);
				}
			}
		strbuf_printf(buf, "}, ");
		strbuf_printf(buf, ".flags = "); dump_tex_flags(buf, fmt_info->flags); strbuf_printf(buf, ", ");
		strbuf_printf(buf, ".bits_per_pixel = %u, ", fmt_info->bits_per_pixel);
	strbuf_printf(buf, "}");
}

static int gl_format_cmp(const void *a, const void *b);

void glcommon_init_texture_formats(void) {
	const bool is_gles = glext.version.is_es;
	const bool is_gles3 = GLES_ATLEAST(3, 0);
	const bool is_gles2 = !is_gles3 && GLES_ATLEAST(2, 0);
	const bool is_angle = glext.version.is_ANGLE;
	const bool have_rg = glext.texture_rg;
	const bool have_uint16 = glext.texture_norm16;
	const bool have_float16 = glext.texture_half_float;
	const bool have_float32 = glext.texture_float;
	const bool have_depth_tex = glext.depth_texture;

	assert(have_rg);

	// TODO: optionally utilize ARB_internalformat_query2 to query actual properties of each format

	GLTextureFormatFlags f_clr_common = GLTEX_COLOR_RENDERABLE | GLTEX_FILTERABLE | GLTEX_BLENDABLE;
	GLTextureFormatFlags f_depth = GLTEX_DEPTH_RENDERABLE;

	// NOTE: in GLES, RGB formats are typically not color-renderable, **except** GL_RGB/GL_RGB8
	GLTextureFormatFlags f_clr_rgb = GLTEX_FILTERABLE | GLTEX_BLENDABLE;

	GLTextureFormatFlags f_clr_float16 = GLTEX_BLENDABLE;
	GLTextureFormatFlags f_clr_float32 = 0;

	GLTextureFormatFlags f_compressed = GLTEX_FILTERABLE | GLTEX_BLENDABLE | GLTEX_COMPRESSED;
	GLTextureFormatFlags f_compressed_srgb = f_compressed | GLTEX_SRGB;

	if(glext.float_blend) {
		f_clr_float32 |= GLTEX_BLENDABLE;
	}

	if(glext.texture_float_linear) {
		f_clr_float32 |= GLTEX_FILTERABLE;
	}

	if(glext.texture_half_float_linear) {
		f_clr_float16 |= GLTEX_FILTERABLE;
	}

	GLTextureFormatFlags f_clr_float16_rgb = f_clr_float16;
	GLTextureFormatFlags f_clr_float32_rgb = f_clr_float32;

	if(glext.color_buffer_float) {
		f_clr_float16 |= GLTEX_COLOR_RENDERABLE;
		f_clr_float32 |= GLTEX_COLOR_RENDERABLE;
	}

	if(!is_gles) {
		f_clr_rgb |= GLTEX_COLOR_RENDERABLE;

		// FIXME: Is there a GLES extension that makes it filterable?
		// It seems to work on Mesa and ANGLE at least.
		f_depth |= GLTEX_FILTERABLE;
	}

	#define ADD(...) \
		(log_debug("ADD: " #__VA_ARGS__), *dynarray_append(&g_formats) = (GLTextureFormatInfo) { __VA_ARGS__ })

	#define ADD_COMPRESSED(comp, basefmt, ifmt, flags) \
		ADD(TEX_TYPE_COMPRESSED_##comp, basefmt, ifmt, { basefmt, GL_BYTE, PIXMAP_FORMAT_##comp }, flags, 0)

	if(is_gles2) {
		ADD(TEX_TYPE_RGBA, GL_RGBA, GL_RGBA, XFER_RGBA8, f_clr_common, 8 * 4);
		ADD(TEX_TYPE_RGB, GL_RGB, GL_RGB, XFER_RGB8, f_clr_common, 8 * 3);

		if(have_depth_tex) {
			ADD(TEX_TYPE_DEPTH_16, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, XFER_DEPTH16, f_depth, 16);
		}
	} else {
		ADD(TEX_TYPE_RGBA, GL_RGBA, GL_RGBA8, XFER_RGBA8, f_clr_common, 8 * 4);
		ADD(TEX_TYPE_RGB, GL_RGB, GL_RGB8, XFER_RGB8, f_clr_common, 8 * 3);

		if(have_depth_tex) {
			if(is_angle) {
				// ANGLE BUG: sized depth formats mysteriously fail. Might've been fixed by now; don't care to check.
				ADD(TEX_TYPE_DEPTH_16, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, XFER_DEPTH16, f_depth, 16);
				ADD(TEX_TYPE_DEPTH_24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, XFER_DEPTH24, f_depth, 24);
				ADD(TEX_TYPE_DEPTH_32_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, XFER_DEPTH32F, f_depth, 32);
			} else {
				ADD(TEX_TYPE_DEPTH_16, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, XFER_DEPTH16, f_depth, 16);
				ADD(TEX_TYPE_DEPTH_24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT24, XFER_DEPTH24, f_depth, 24);
				ADD(TEX_TYPE_DEPTH_32_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F, XFER_DEPTH32F, f_depth, 32);
			}
		}
	}

	if(have_rg) {
		ADD(TEX_TYPE_R_8, GL_RED, GL_R8, XFER_R8, f_clr_common, 8 * 1);
		ADD(TEX_TYPE_RG_8, GL_RG, GL_RG8, XFER_RG8, f_clr_common, 8 * 2);

		if(have_uint16) {
			ADD(TEX_TYPE_R_16, GL_RED, GL_R16, XFER_R16, f_clr_common, 16 * 1);
			ADD(TEX_TYPE_RG_16, GL_RG, GL_RG16, XFER_RG16, f_clr_common, 16 * 2);
		}

		if(have_float16) {
			ADD(TEX_TYPE_R_16_FLOAT, GL_RED, GL_R16F, XFER_RG32F, f_clr_float16, 16 * 1);
			ADD(TEX_TYPE_RG_16_FLOAT, GL_RG, GL_RG16F, XFER_RG32F, f_clr_float16, 16 * 2);
		}

		if(have_float32) {
			ADD(TEX_TYPE_R_32_FLOAT, GL_RED, GL_R32F, XFER_RG32F, f_clr_float32, 32 * 1);
			ADD(TEX_TYPE_RG_32_FLOAT, GL_RG, GL_RG32F, XFER_RG32F, f_clr_float32, 32 * 2);
		}
	}

	if(have_uint16) {
		ADD(TEX_TYPE_RGBA_16, GL_RGBA, GL_RGBA16, XFER_RGBA16, f_clr_common, 16 * 4);
		ADD(TEX_TYPE_RGB_16, GL_RGB, GL_RGB16, XFER_RGB16, f_clr_rgb, 16 * 3);
	}

	if(have_float16) {
		ADD(TEX_TYPE_RGBA_16_FLOAT, GL_RGBA, GL_RGBA16F, XFER_RGBA32F, f_clr_float16, 16 * 4);
		ADD(TEX_TYPE_RGB_16_FLOAT, GL_RGB, GL_RGB16F, XFER_RGB32F, f_clr_float16_rgb, 16 * 3);
	}

	if(have_float32) {
		ADD(TEX_TYPE_RGBA_32_FLOAT, GL_RGBA, GL_RGBA32F, XFER_RGBA32F, f_clr_float32, 32 * 4);
		ADD(TEX_TYPE_RGB_32_FLOAT, GL_RGB, GL_RGB32F, XFER_RGB32F, f_clr_float32_rgb, 32 * 3);
	}

	if(glext.tex_format.r8_srgb) {
		ADD(TEX_TYPE_R_8, GL_RED, GL_SR8_EXT, XFER_R8, f_clr_common | GLTEX_SRGB, 8 * 1);
	}

	if(glext.tex_format.rg8_srgb) {
		ADD(TEX_TYPE_RG_8, GL_RG, GL_SRG8_EXT, XFER_RG8, f_clr_common | GLTEX_SRGB, 8 * 2);
	}

	if(glext.tex_format.rgb8_rgba8_srgb) {
		ADD(TEX_TYPE_RGBA_8, GL_RGBA, GL_SRGB8_ALPHA8, XFER_RGBA8, f_clr_common | GLTEX_SRGB, 8 * 4);
		ADD(TEX_TYPE_RGB_8, GL_RGB, GL_SRGB8, XFER_RGB8, f_clr_rgb | GLTEX_SRGB, 8 * 3);
	}

	if(glext.tex_format.astc) {
		ADD_COMPRESSED(ASTC_4x4_RGBA, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_4x4_KHR, f_compressed);
		ADD_COMPRESSED(ASTC_4x4_RGBA, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, f_compressed_srgb);
	}

	if(glext.tex_format.s3tc_dx1) {
		ADD_COMPRESSED(BC1_RGB, GL_RGB, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, f_compressed);

		if(glext.tex_format.s3tc_srgb) {
			ADD_COMPRESSED(BC1_RGB, GL_RGB, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, f_compressed_srgb);
		}
	}

	if(glext.tex_format.s3tc_dx5) {
		ADD_COMPRESSED(BC3_RGBA, GL_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, f_compressed);

		if(glext.tex_format.s3tc_srgb) {
			ADD_COMPRESSED(BC3_RGBA, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, f_compressed_srgb);
		}
	}

	if(glext.tex_format.rgtc) {
		ADD_COMPRESSED(BC4_R, GL_RED, GL_COMPRESSED_RED_RGTC1, f_compressed);
		ADD_COMPRESSED(BC5_RG, GL_RG, GL_COMPRESSED_RG_RGTC2, f_compressed);
	}

	if(glext.tex_format.bptc) {
		ADD_COMPRESSED(BC7_RGBA, GL_RGBA, GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, f_compressed);
		ADD_COMPRESSED(BC7_RGBA, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, f_compressed_srgb);
	}

	if(glext.tex_format.etc1) {
		ADD_COMPRESSED(ETC1_RGB, GL_RGB, GL_ETC1_RGB8_OES, f_compressed);

		if(glext.tex_format.etc1_srgb) {
			ADD_COMPRESSED(ETC1_RGB, GL_RGB, GL_ETC1_SRGB8_NV, f_compressed_srgb);
		}
	}

	if(glext.tex_format.etc2_eac) {
		ADD_COMPRESSED(ETC2_RGBA, GL_RGBA, GL_COMPRESSED_RGBA8_ETC2_EAC, f_compressed);
		ADD_COMPRESSED(ETC2_EAC_R11, GL_RED, GL_COMPRESSED_R11_EAC, f_compressed);
		ADD_COMPRESSED(ETC2_EAC_RG11, GL_RG, GL_COMPRESSED_RG11_EAC, f_compressed);

		// NOTE: ETC2 is backwards compatible with ETC1
		ADD_COMPRESSED(ETC1_RGB, GL_RGB, GL_COMPRESSED_RGB8_ETC2, f_compressed);

		if(glext.tex_format.etc2_eac_srgb) {
			ADD_COMPRESSED(ETC2_RGBA, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, f_compressed_srgb);
			ADD_COMPRESSED(ETC1_RGB, GL_RGB, GL_COMPRESSED_SRGB8_ETC2, f_compressed_srgb);
		}
	}

	if(glext.tex_format.pvrtc) {
		ADD_COMPRESSED(PVRTC1_4_RGB, GL_RGB, GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, f_compressed);
		ADD_COMPRESSED(PVRTC1_4_RGBA, GL_RGBA, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, f_compressed);

		if(glext.tex_format.pvrtc_srgb) {
			ADD_COMPRESSED(PVRTC1_4_RGB, GL_RGB, GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT, f_compressed_srgb);
			ADD_COMPRESSED(PVRTC1_4_RGBA, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT, f_compressed_srgb);
		}
	}

	if(glext.tex_format.pvrtc2) {
		// NOTE: I'm not sure how this works, but according to the following table, both RGB and RGBA variants of
		// PVRTCv2 map to the RGBA OpenGL enum (and there is no RGB, one anyway):
		// https://github.com/BinomialLLC/basis_universal/wiki/OpenGL-texture-format-enums-table

		ADD_COMPRESSED(PVRTC2_4_RGBA, GL_RGBA, GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, f_compressed);
		ADD_COMPRESSED(PVRTC2_4_RGB, GL_RGBA, GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, f_compressed);

		if(glext.tex_format.pvrtc_srgb) {
			ADD_COMPRESSED(PVRTC2_4_RGBA, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG, f_compressed_srgb);
			ADD_COMPRESSED(PVRTC2_4_RGB, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG, f_compressed_srgb);
		}
	}

	if(glext.tex_format.atc) {
		ADD_COMPRESSED(ATC_RGB, GL_RGB, GL_ATC_RGB_AMD, f_compressed);
		ADD_COMPRESSED(ATC_RGBA, GL_RGBA, GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD, f_compressed);
	}

	if(glext.tex_format.fxt1) {
		ADD_COMPRESSED(FXT1_RGB, GL_RGB, GL_COMPRESSED_RGB_FXT1_3DFX, f_compressed);
	}

	dynarray_compact(&g_formats);
	dynarray_qsort(&g_formats, gl_format_cmp);
}

void glcommon_free_texture_formats(void) {
	dynarray_free_data(&g_formats);
}

static int gl_format_cmp(const void *a, const void *b) {
	const GLTextureFormatInfo *fmt_info_a = a;
	const GLTextureFormatInfo *fmt_info_b = b;
	return fmt_info_a->intended_type_mapping - fmt_info_b->intended_type_mapping;
}

static int gl_format_search_cmp(const void *vkey, const void *velem) {
	TextureType key = (uintptr_t)vkey;
	const GLTextureFormatInfo *elem = velem;
	return key - elem->intended_type_mapping;
}

static GLTextureFormatInfo *find_first(TextureType tex_type) {
	GLTextureFormatInfo *fmt_info = bsearch((void*)tex_type, g_formats.data, g_formats.num_elements, sizeof(*g_formats.data), gl_format_search_cmp);

	if(fmt_info) {
		assert(fmt_info->intended_type_mapping == tex_type);
		while(fmt_info > g_formats.data && fmt_info[-1].intended_type_mapping == tex_type) {
			--fmt_info;
		}
	}

	return fmt_info;
}

static GLTextureFormatInfo *find_format(TextureType tex_type, GLTextureFormatFlags require_flags, GLTextureFormatFlags forbid_flags) {
	GLTextureFormatInfo *fmt_info = find_first(tex_type);

	if(fmt_info == NULL) {
		return NULL;
	}

	GLTextureFormatInfo *end = g_formats.data + g_formats.num_elements;

	do {
		if(
			(fmt_info->flags & require_flags) == require_flags &&
			(fmt_info->flags & forbid_flags)  == 0
		) {
			return fmt_info;
		}

		++fmt_info;
	} while(fmt_info < end && fmt_info->intended_type_mapping == tex_type);

	return NULL;
}

static int glfmt_num_chans(GLenum basefmt) {
	switch(basefmt) {
		case GL_RED  : return 1;
		case GL_RG   : return 2;
		case GL_RGB  : return 3;
		case GL_RGBA : return 4;
		case GL_DEPTH_COMPONENT : return 1;
		default: UNREACHABLE;
	}
}

static int textype_num_chans(TextureType type) {
	static uint8_t map[] = {
		#define HANDLE(chans, numchans) \
			[TEX_TYPE_##chans##_8] = numchans, \
			[TEX_TYPE_##chans##_16] = numchans, \
			[TEX_TYPE_##chans##_16_FLOAT] = numchans, \
			[TEX_TYPE_##chans##_32_FLOAT] = numchans \

		HANDLE(RGBA, 4),
		HANDLE(RGB, 3),
		HANDLE(RG, 2),
		HANDLE(R, 1),
		HANDLE(DEPTH, 1),

		[TEX_TYPE_DEPTH_24] = 1,
		[TEX_TYPE_DEPTH_32] = 1,

		#undef HANDLE
	};

	uint idx = type;
	assume(idx < ARRAY_SIZE(map));
	return map[idx];
}

static int textype_bits_per_pixel(TextureType type) {
	static uint8_t map[] = {
		#define HANDLE(chans, numchans) \
		[TEX_TYPE_##chans##_8] = numchans * 8, \
		[TEX_TYPE_##chans##_16] = numchans * 16, \
		[TEX_TYPE_##chans##_16_FLOAT] = numchans * 16, \
		[TEX_TYPE_##chans##_32_FLOAT] = numchans * 32 \

		HANDLE(RGBA, 4),
		HANDLE(RGB, 3),
		HANDLE(RG, 2),
		HANDLE(R, 1),
		HANDLE(DEPTH, 1),

		[TEX_TYPE_DEPTH_24] = 24,
		[TEX_TYPE_DEPTH_32] = 32,

		#undef HANDLE
	};

	uint idx = type;
	assume(idx < ARRAY_SIZE(map));
	return map[idx];
}

GLTextureFormatInfo *glcommon_match_format(TextureType tex_type, GLTextureFormatFlags require_flags, GLTextureFormatFlags forbid_flags) {
	assert((require_flags & forbid_flags) == 0);

	bool compressed = TEX_TYPE_IS_COMPRESSED(tex_type);

	if(compressed) {
		assume(require_flags & GLTEX_COMPRESSED);
	} else {
		assume(forbid_flags & GLTEX_COMPRESSED);
	}

	GLTextureFormatInfo *exact_match = find_format(tex_type, require_flags, forbid_flags);

	if(exact_match || compressed) {
		return exact_match;
	}

	int ideal_chans = textype_num_chans(tex_type);
	int ideal_bits = textype_bits_per_pixel(tex_type);
	int ideal_bpc_score = (ideal_bits << 4) / ideal_chans;

	GLTextureFormatInfo *best = NULL;
	int best_fitness = 0;

	StringBuffer buf = { 0 };
	#define LOGBUF(...) strbuf_printf(&buf, __VA_ARGS__)
	#define LOGBUF_FLUSH() do { log_debug("%s", buf.start); strbuf_clear(&buf); } while(0)

	dynarray_foreach(&g_formats, int i, GLTextureFormatInfo *fmt_info, {
		log_debug("ITER #%i", i);

		LOGBUF("CONSIDER: ");
		dump_fmt_info(&buf, fmt_info);
		LOGBUF_FLUSH();

		if((fmt_info->flags & require_flags) != require_flags) {
			LOGBUF("REJECT: missing flags: ");
			dump_tex_flags(&buf, require_flags & ~(fmt_info->flags & require_flags));
			LOGBUF_FLUSH();
			continue;
		}

		if((fmt_info->flags & forbid_flags) != 0) {
			LOGBUF("REJECT: forbidden flags: ");
			dump_tex_flags(&buf, fmt_info->flags & forbid_flags);
			LOGBUF_FLUSH();
			continue;
		}

		assert(fmt_info->intended_type_mapping != tex_type);

		if(compressed) {
			continue;
		}

		int fmt_chans = glfmt_num_chans(fmt_info->base_format);

		if(fmt_chans < ideal_chans) {
			log_debug("REJECT: intended type match");
			continue;
		}

		int fmt_bits = fmt_info->bits_per_pixel;
		int bpc_score = (fmt_bits << 4) / fmt_chans;
		int excess_chans = fmt_chans - ideal_chans;

		int fitness = ((5 - excess_chans) << 10);

		if(bpc_score >= ideal_bpc_score) {
			fitness += (1 << 13) - (fmt_bits - ideal_bits);
		} else {
			fitness += bpc_score;
		}

		if(fitness > best_fitness) {
			log_debug("BEST: fitness = %i; prev best = %i", fitness, best_fitness);
			best = fmt_info;
			best_fitness = fitness;
		} else {
			log_debug("WEAK: fitness = %i; best = %i", fitness, best_fitness);
		}
	});

	if(best) {
		LOGBUF("PICK: ");
		dump_fmt_info(&buf, best);
		LOGBUF_FLUSH();
	} else {
		log_debug("NO MATCH!");
	}

	strbuf_free(&buf);
	return best;
}
