/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "texture_loader.h"
#include "basisu.h"

void texture_loader_cleanup_stage1(TextureLoadData *ld) {
	free(ld->tex_src_path_allocated);
	ld->tex_src_path_allocated = NULL;

	free(ld->alphamap_src_path_allocated);
	ld->alphamap_src_path_allocated = NULL;
}

void texture_loader_cleanup_stage2(TextureLoadData *ld) {
	free(ld->pixmap.data.untyped);
	ld->pixmap.data.untyped = NULL;

	free(ld->pixmap_alphamap.data.untyped);
	ld->pixmap_alphamap.data.untyped = NULL;

	if(ld->mipmaps) {
		for(uint i = 0; i < ld->num_mipmaps; ++i) {
			free(ld->mipmaps[i].data.untyped);
		}

		free(ld->mipmaps);
		ld->mipmaps = NULL;
		ld->num_mipmaps = 0;
	}
}

void texture_loader_cleanup(TextureLoadData *ld) {
	texture_loader_cleanup_stage1(ld);
	texture_loader_cleanup_stage2(ld);
	free(ld);
}

void texture_loader_failed(TextureLoadData *ld) {
	res_load_failed(ld->st);
	texture_loader_cleanup(ld);
}

char *texture_loader_source_path(const char *basename) {
	char *p = NULL;

	if((p = texture_loader_basisu_try_path(basename))) {
		return p;
	}

	return pixmap_source_path(TEX_PATH_PREFIX, basename);
}

char *texture_loader_path(const char *basename) {
	char *p = NULL;

	if((p = try_path(TEX_PATH_PREFIX, basename, TEX_EXTENSION))) {
		return p;
	}

	return texture_loader_source_path(basename);
}

bool texture_loader_check_path(const char *path) {
	return
		strendswith(path, TEX_EXTENSION) ||
		texture_loader_basisu_check_path(path) ||
		pixmap_check_filename(path);
}

static void texture_loader_parse_filter(
	TextureLoadData *ld,
	const char *field_name,
	const char *val,
	TextureFilterMode *out,
	bool allow_mipmaps
) {
	if(!val) {
		return;
	}

	char buf[strlen(val) + 1];
	strcpy(buf, val);
	char *ignore, *pbuf = strtok_r(buf, " \t", &ignore);

	if(!SDL_strcasecmp(pbuf, "default")) {
		return;
	}

	if(!SDL_strcasecmp(pbuf, "nearest")) {
		*out = TEX_FILTER_NEAREST;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "linear")) {
		*out = TEX_FILTER_LINEAR;
		return;
	}

	if(allow_mipmaps) {
		if(!SDL_strcasecmp(pbuf, "nearest_mipmap_nearest")) {
			*out = TEX_FILTER_NEAREST_MIPMAP_NEAREST;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "nearest_mipmap_linear")) {
			*out = TEX_FILTER_NEAREST_MIPMAP_LINEAR;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "linear_mipmap_nearest")) {
			*out = TEX_FILTER_LINEAR_MIPMAP_NEAREST;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "linear_mipmap_linear")) {
			*out = TEX_FILTER_LINEAR_MIPMAP_LINEAR;
			return;
		}
	}

	log_warn("%s: Bad %s value `%s`, assuming default", ld->st->name, field_name, pbuf);
}

static void texture_loader_parse_warp(
	TextureLoadData *ld,
	const char *field_name,
	const char *val,
	TextureWrapMode *out
) {
	if(!val) {
		return;
	}

	char buf[strlen(val) + 1];
	strcpy(buf, val);
	char *ignore, *pbuf = strtok_r(buf, " \t", &ignore);

	if(!SDL_strcasecmp(pbuf, "default")) {
		return;
	}

	if(!SDL_strcasecmp(pbuf, "clamp")) {
		*out = TEX_WRAP_CLAMP;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "mirror")) {
		*out = TEX_WRAP_MIRROR;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "repeat")) {
		*out = TEX_WRAP_REPEAT;
		return;
	}

	log_warn("%s: Bad %s value `%s`, assuming default", ld->st->name, field_name, pbuf);
}

static bool texture_loader_parse_format(
	TextureLoadData *ld,
	const char *val,
	PixmapFormat *out
) {
	if(!val) {
		return true;
	}

	uint channels = 0;

	char buf[strlen(val) + 1];
	strcpy(buf, val);

	if(*val) {
		char *c = strrchr(buf, 0) - 1;
		while(isspace(*c) && c >= buf) {
			*c-- = 0;
		}
	}

	val = buf;

	for(uint i = 4; i >= 1; --i) {
		if(!SDL_strncasecmp(val, "rgba", i)) {
			channels = i;
			break;
		}
	}

	if(channels < 1) {
		log_error("%s: Invalid format '%s': Bad channels specification, expected one of: RGBA, RGB, RG, R", ld->st->name, val);
		return false;
	}

	assert(channels <= 4);

	char *end;
	uint depth = strtol(val + channels, &end, 10);

	if(val + channels == end) {
		log_error("%s: Invalid format '%s': Bad bit depth specification, expected an integer", ld->st->name, val);
		return false;
	}

	if(depth != 8 && depth != 16 && depth != 32) {
		log_error("%s: Invalid format '%s': Invalid bit depth %d, only 8, 16, and 32 are supported", ld->st->name, val, depth);
		return false;
	}

	while(isspace(*end)) {
		++end;
	}

	bool is_float = false;

	if(*end) {
		if(!SDL_strcasecmp(end, "u") || !SDL_strcasecmp(end, "uint")) {
			// is_float = false;
		} else if(!SDL_strcasecmp(end, "f") || !SDL_strcasecmp(end, "float")) {
			is_float = true;
		} else {
			log_error("%s: Invalid format '%s': Bad data type specification, expected one of: U, UINT, F, FLOAT, or nothing", ld->st->name, val);
			return false;
		}
	}

	if(depth == 32 && !is_float) {
		log_error("%s: Invalid format '%s': Bit depth %d is only supported for floating point data", ld->st->name, val, depth);
		return false;
	} else if(depth < 16 && is_float) {
		log_error("%s: Invalid format '%s': Bit depth %d is not supported for floating point data", ld->st->name, val, depth);
		return false;
	}

	*out = PIXMAP_MAKE_FORMAT(channels, depth) | (is_float * PIXMAP_FLOAT_BIT);
	return true;
}

bool texture_loader_try_set_texture_type(
	TextureLoadData *ld,
	TextureType tex_type,
	PixmapFormat px_fmt,
	PixmapOrigin px_org,
	bool srgb_fallback,
	TextureTypeQueryResult *out_qr
) {
	TextureTypeQueryResult qr;
	bool type_supported = r_texture_type_query(tex_type, ld->params.flags, px_fmt, px_org, &qr);

	if(!type_supported && (ld->params.flags & TEX_FLAG_SRGB) && srgb_fallback) {
		ld->params.flags &= ~TEX_FLAG_SRGB;
		ld->preprocess.linearize = true;
		type_supported = r_texture_type_query(tex_type, ld->params.flags, px_fmt, px_org, &qr);
	}

	if(type_supported) {
		ld->params.type = tex_type;

		if(out_qr) {
			*out_qr = qr;
		}

		return true;
	}

	return false;
}

bool texture_loader_set_texture_type_uncompressed(
	TextureLoadData *ld,
	TextureType tex_type,
	PixmapFormat px_fmt,
	PixmapOrigin px_org,
	TextureTypeQueryResult *out_qr
) {
	assert(!TEX_TYPE_IS_COMPRESSED(tex_type));
	bool ok = texture_loader_try_set_texture_type(ld, tex_type, px_fmt, px_org, true, out_qr);

	if(!ok) {
		log_error("%s: Texture type %s not supported", ld->st->name, r_texture_type_name(tex_type));
		return false;
	}

	if(ld->preprocess.linearize) {
		log_warn("%s: No native sRGB support; will convert texture into linear colorspace", ld->st->name);
	}

	return true;
}

bool texture_loader_prepare_pixmaps(
	TextureLoadData *ld,
	Pixmap *pm_main,
	Pixmap *pm_alphamap,
	TextureType tex_type,
	TextureFlags tex_flags
) {
	TextureTypeQueryResult qr;
	if(!r_texture_type_query(tex_type, tex_flags, pm_main->format, pm_main->origin, &qr)) {
		log_error("%s: Texture type %s not supported", ld->st->name, r_texture_type_name(tex_type));
		return false;
	}

	pixmap_convert_inplace_realloc(pm_main, qr.optimal_pixmap_format);
	pixmap_flip_to_origin_inplace(pm_main, qr.optimal_pixmap_origin);

	if(pm_alphamap && pm_alphamap->data.untyped) {
		TextureType alphamap_type;

		if(PIXMAP_FORMAT_DEPTH(pm_alphamap->format) > 8) {
			alphamap_type = TEX_TYPE_R_16;
		} else {
			alphamap_type = TEX_TYPE_R_8;
		}

		if(!r_texture_type_query(alphamap_type, 0, pm_alphamap->format, pm_alphamap->origin, &qr)) {
			log_error("%s: Alphamap texture type %s not supported", ld->st->name, r_texture_type_name(alphamap_type));
			return false;
		}

		pixmap_convert_inplace_realloc(pm_alphamap, qr.optimal_pixmap_format);
		pixmap_flip_to_origin_inplace(pm_alphamap, qr.optimal_pixmap_origin);
	}

	return true;
}

void texture_loader_continue(TextureLoadData *ld);

static bool is_preprocess_needed(TextureLoadData *ld) {
	return (
		ld->preprocess.linearize |
		ld->preprocess.multiply_alpha |
		ld->preprocess.apply_alphamap |
		0
	);
}

static void texture_loader_stage2(ResourceLoadState *st);

void texture_loader_stage1(ResourceLoadState *st) {
	TextureLoadData *ld = malloc(sizeof(*ld));
	*ld = (TextureLoadData) {
		.params = {
			.filter = {
				.mag = TEX_FILTER_LINEAR,
				.min = TEX_FILTER_LINEAR_MIPMAP_LINEAR,
			},
			.wrap = {
				.s = TEX_WRAP_REPEAT,
				.t = TEX_WRAP_REPEAT,
			},
			.mipmaps = TEX_MIPMAPS_MAX,
			.anisotropy = TEX_ANISOTROPY_DEFAULT,
		},
		.preprocess.multiply_alpha = true,
		.st = st,
		.tex_src_path = st->path,
	};

	bool want_srgb = false;

	if(strendswith(st->path, TEX_EXTENSION)) {
		char *str_filter_min = NULL;
		char *str_filter_mag = NULL;
		char *str_wrap_s = NULL;
		char *str_wrap_t = NULL;
		char *str_format = NULL;

		if(!parse_keyvalue_file_with_spec(st->path, (KVSpec[]) {
			{ "source",         .out_str  = &ld->tex_src_path_allocated },
			{ "alphamap",       .out_str  = &ld->alphamap_src_path_allocated },
			{ "filter_min",     .out_str  = &str_filter_min },
			{ "filter_mag",     .out_str  = &str_filter_mag },
			{ "wrap_s",         .out_str  = &str_wrap_s },
			{ "wrap_t",         .out_str  = &str_wrap_t },
			{ "format",         .out_str  = &str_format },
			{ "mipmaps",        .out_int  = (int*)&ld->params.mipmaps },
			{ "anisotropy",     .out_int  = (int*)&ld->params.anisotropy },
			{ "multiply_alpha", .out_bool = &ld->preprocess.multiply_alpha },
			{ "linearize",      .out_bool = &want_srgb },
			{ NULL }
		})) {
			texture_loader_failed(ld);
			return;
		}

		if(!ld->tex_src_path_allocated) {
			ld->tex_src_path_allocated = texture_loader_source_path(st->name);

			if(!ld->tex_src_path_allocated) {
				log_error("%s: Couldn't infer source path from texture name", st->name);
			} else {
				log_info("%s: Inferred source path from texture name: %s", st->name, ld->tex_src_path_allocated);
			}

			if(!ld->tex_src_path_allocated) {
				texture_loader_failed(ld);
				return;
			}
		}

		ld->tex_src_path = ld->tex_src_path_allocated;
		ld->alphamap_src_path = ld->alphamap_src_path_allocated;

		texture_loader_parse_filter(ld, "filter_min", str_filter_min, &ld->params.filter.min, true);
		free(str_filter_min);

		texture_loader_parse_filter(ld, "filter_mag", str_filter_mag, &ld->params.filter.mag, false);
		free(str_filter_mag);

		texture_loader_parse_warp(ld, "warp_s", str_wrap_s, &ld->params.wrap.s);
		free(str_wrap_s);

		texture_loader_parse_warp(ld, "warp_t", str_wrap_t, &ld->params.wrap.t);
		free(str_wrap_t);

		bool format_ok = texture_loader_parse_format(ld, str_format, &ld->preferred_format);
		free(str_format);

		if(!format_ok) {
			log_error("%s: Bad or unsupported pixel format specification", st->path);
			texture_loader_failed(ld);
			return;
		}
	}

	if(want_srgb) {
		ld->params.flags |= TEX_FLAG_SRGB;
	}

	assert(!ld->preprocess.linearize);

	if(texture_loader_basisu_check_path(ld->tex_src_path)) {
		free(ld->alphamap_src_path_allocated);
		ld->alphamap_src_path = ld->alphamap_src_path_allocated = NULL;
		texture_loader_basisu(ld);
		return;
	}

	if(!pixmap_load_file(ld->tex_src_path, &ld->pixmap, ld->preferred_format)) {
		log_error("%s: Couldn't load texture image %s", st->name, ld->tex_src_path);
		texture_loader_failed(ld);
		return;
	}

	if(ld->alphamap_src_path && !pixmap_load_file(ld->alphamap_src_path, &ld->pixmap_alphamap, PIXMAP_FORMAT_R8)) {
		log_error("%s: Couldn't load texture alphamap %s", st->name, ld->alphamap_src_path);
		texture_loader_failed(ld);
		return;
	}

	ld->preferred_format = ld->preferred_format ? ld->preferred_format : ld->pixmap.format;
	TextureType tex_type = r_texture_type_from_pixmap_format(ld->preferred_format);
	bool apply_format_ok = texture_loader_set_texture_type_uncompressed(ld, tex_type, ld->pixmap.format, ld->pixmap.origin, NULL);

	if(!apply_format_ok) {
		texture_loader_failed(ld);
		return;
	}

	if(!texture_loader_prepare_pixmaps(ld, &ld->pixmap, &ld->pixmap_alphamap, ld->params.type, ld->params.flags)) {
		texture_loader_failed(ld);
		return;
	}

	if(ld->pixmap_alphamap.data.untyped) {
		ld->preprocess.apply_alphamap = true;
	}

	if(pixmap_format_layout(ld->preferred_format) != PIXMAP_LAYOUT_RGBA) {
		ld->preprocess.multiply_alpha = false;
	}

	ld->params.width = ld->pixmap.width;
	ld->params.height = ld->pixmap.height;

	texture_loader_continue(ld);
}

void texture_loader_continue(TextureLoadData *ld) {
	texture_loader_cleanup_stage1(ld);
	bool preprocess_needed = is_preprocess_needed(ld);

	if(TEX_TYPE_IS_COMPRESSED(ld->params.type)) {
		assert(!preprocess_needed);

		if(preprocess_needed) {
			log_warn("%s: Preprocessing not implemented for compressed textures", ld->st->name);
			memset(&ld->preprocess, 0, sizeof(ld->preprocess));
		}
	}

	if(ld->mipmaps) {
		assert(!preprocess_needed);

		if(preprocess_needed) {
			log_warn("%s: Preprocessing not implemented for textures with explicit mipmaps", ld->st->name);
			memset(&ld->preprocess, 0, sizeof(ld->preprocess));
		}

		assume(ld->pixmap_alphamap.data.untyped == NULL);
		assert(ld->params.mipmap_mode == TEX_MIPMAP_MANUAL);
	} else {
		ld->params.mipmap_mode = TEX_MIPMAP_AUTO;
	}

	if(ld->params.mipmaps == 0) {
		ld->params.mipmaps = TEX_MIPMAPS_MAX;
	}

	if(ld->params.mipmaps == 1) {
		// Special case: one mip level (no mipmaps).
		// Disable mipmap generation and filtering modes.

		log_debug("%s: Texture has only 1 mip level, adjusting filters", ld->st->name);

		switch(ld->params.filter.min) {
			case TEX_FILTER_LINEAR_MIPMAP_LINEAR:
			case TEX_FILTER_LINEAR_MIPMAP_NEAREST:
				log_debug("%s: Min filter changed to linear", ld->st->name);
				ld->params.filter.min = TEX_FILTER_LINEAR;
				break;

			case TEX_FILTER_NEAREST_MIPMAP_LINEAR:
			case TEX_FILTER_NEAREST_MIPMAP_NEAREST:
				log_debug("%s: Min filter changed to nearest", ld->st->name);
				ld->params.filter.min = TEX_FILTER_NEAREST;
				break;

			default:
				break;
		}

		ld->params.mipmap_mode = TEX_MIPMAP_MANUAL;
	}

	res_load_continue_on_main(ld->st, texture_loader_stage2, ld);
}

static Texture *texture_loader_preprocess(
	TextureLoadData *ld,
	Texture *tex,
	Texture *alphamap
);

static void texture_loader_stage2(ResourceLoadState *st) {
	TextureLoadData *ld = NOT_NULL(st->opaque);
	assume(ld->st == st);

	bool preprocess_needed = is_preprocess_needed(ld);

	if(TEX_TYPE_IS_COMPRESSED(ld->params.type)) {
		assert(!preprocess_needed);
	}

	if(ld->mipmaps) {
		assert(!preprocess_needed);
		assert(ld->pixmap_alphamap.data.untyped == NULL);
		assert(ld->params.mipmap_mode == TEX_MIPMAP_MANUAL);
		assert(ld->num_mipmaps > 0);
	} else {
		assert(ld->num_mipmaps == 0);
	}

	Texture *texture;

	if(preprocess_needed) {
		// Create transient texture that we'll just render into another one and discard.
		// Doesn't need mipmaps and filtering.

		TextureParams p = ld->params;
		p.mipmaps = 1;
		p.mipmap_mode = TEX_MIPMAP_MANUAL;
		p.filter.min = TEX_FILTER_NEAREST;
		p.filter.mag = TEX_FILTER_NEAREST;

		texture = r_texture_create(&p);

		char namebuf[strlen(st->name) + sizeof(" (transient)")];
		snprintf(namebuf, sizeof(namebuf), "%s (transient)", st->name);
		r_texture_set_debug_label(texture, namebuf);
	} else {
		texture = r_texture_create(&ld->params);
		r_texture_set_debug_label(texture, st->name);
	}

	r_texture_fill(texture, 0, &ld->pixmap);
	free(ld->pixmap.data.untyped);
	ld->pixmap.data.untyped = NULL;

	for(uint i = 0; i < ld->num_mipmaps; ++i) {
		r_texture_fill(texture, i + 1, &ld->mipmaps[i]);
		free(ld->mipmaps[i].data.untyped);
	}

	free(ld->mipmaps);
	ld->mipmaps = NULL;

	Texture *alphamap = NULL;

	if(ld->pixmap_alphamap.data.untyped) {
		assume(ld->preprocess.apply_alphamap);
		assume(preprocess_needed);
		TextureParams p = ld->params;
		p.type = r_texture_type_from_pixmap_format(ld->pixmap_alphamap.format);
		p.mipmaps = 1;
		p.mipmap_mode = TEX_MIPMAP_MANUAL;
		p.filter.min = TEX_FILTER_NEAREST;
		p.filter.mag = TEX_FILTER_NEAREST;
		alphamap = r_texture_create(&p);
		const char suffix[] = " alphamap";
		char buf[strlen(st->name) + sizeof(suffix)];
		snprintf(buf, sizeof(buf), "%s%s", st->name, suffix);
		r_texture_set_debug_label(alphamap, buf);
		r_texture_fill(alphamap, 0, &ld->pixmap_alphamap);
		free(ld->pixmap_alphamap.data.untyped);
		ld->pixmap_alphamap.data.untyped = NULL;
	}

	if(preprocess_needed) {
		texture = texture_loader_preprocess(ld, texture, alphamap);
		r_texture_set_debug_label(texture, st->name);
	}

	if(!strcmp(st->name, "atlas_common_0")) {
		assert(preprocess_needed);
		assert(ld->params.mipmap_mode == TEX_MIPMAP_AUTO);
		assert(ld->params.mipmaps == TEX_MIPMAPS_MAX);
	}

	texture_loader_cleanup(ld);

	if(alphamap) {
		r_texture_destroy(alphamap);
	}

	res_load_finished(st, texture);
}

static Texture *texture_loader_preprocess(
	TextureLoadData *ld,
	Texture *tex,
	Texture *alphamap
) {
	Texture *fbo_tex;
	Framebuffer *fb;

	r_state_push();
	r_blend(BLEND_NONE);
	r_disable(RCAP_CULL_FACE);
	fbo_tex = r_texture_create(&ld->params);

	r_shader("texture_post_load");
	r_uniform_sampler("tex", tex);

	r_uniform_int("linearize", ld->preprocess.linearize);
	r_uniform_int("multiply_alpha", ld->preprocess.multiply_alpha);
	r_uniform_int("apply_alphamap", ld->preprocess.apply_alphamap);
	r_uniform_int("write_srgb", (ld->params.flags & TEX_FLAG_SRGB) == TEX_FLAG_SRGB);

	if(alphamap) {
		r_uniform_sampler("alphamap", alphamap);
	} else {
		// FIXME: even though we aren't going to actually use this sampler, it
		// must be set to something valid, lest WebGL throw a fit. This should be
		// worked around in the renderer code, not here.
		r_uniform_sampler("alphamap", tex);
	}

	r_mat_mv_push_identity();
	r_mat_proj_push_ortho(ld->params.width, ld->params.height);
	r_mat_mv_scale(ld->params.width, ld->params.height, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	fb = r_framebuffer_create();
	r_framebuffer_attach(fb, fbo_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(fb, 0, 0, ld->params.width, ld->params.height);
	r_framebuffer(fb);
	r_draw_quad();
	r_mat_mv_pop();
	r_mat_proj_pop();
	r_framebuffer_destroy(fb);
	r_texture_destroy(tex);
	r_state_pop();

	return fbo_tex;
}

void texture_loader_unload(void *vtexture) {
	r_texture_destroy(vtexture);
}
