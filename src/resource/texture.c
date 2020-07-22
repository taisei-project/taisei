/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "texture.h"
#include "resource.h"
#include "global.h"
#include "video.h"
#include "renderer/api.h"
#include "util/pixmap.h"

#include <basisu_transcoder_c_api.h>

static void load_texture_stage1(ResourceLoadState *st);
static void load_texture_stage2(ResourceLoadState *st);
static void free_texture(Texture *tex);

ResourceHandler texture_res_handler = {
	.type = RES_TEXTURE,
	.typename = "texture",
	.subdir = TEX_PATH_PREFIX,

	.procs = {
		.find = texture_path,
		.check = check_texture_path,
		.load = load_texture_stage1,
		.unload = (ResourceUnloadProc)free_texture,
	},
};

static void basist_tls_destructor(void *tc) {
	basist_transcoder_destroy(NOT_NULL(tc));
}

static basist_transcoder *get_basis_transcoder() {
	static SDL_SpinLock lock;
	static SDL_TLSID tls;
	static basist_transcoder *fallback;

	if(UNLIKELY(fallback)) {
		return fallback;
	}

	if(UNLIKELY(!tls)) {
		SDL_AtomicLock(&lock);
		if(!tls) {
			tls = SDL_TLSCreate();
		}
		SDL_AtomicUnlock(&lock);
	}

	basist_transcoder *tc;

	if(LIKELY(tls)) {
		tc = SDL_TLSGet(tls);
	} else {
		tc = fallback;
	}

	if(LIKELY(tc)) {
		return tc;
	}

	tc = basist_transcoder_create();

	if(UNLIKELY(tc == NULL)) {
		log_error("basist_transcoder_create() failed");
		return NULL;
	}

	log_debug("Created Basis Universal transcoder %p for thread %p", (void*)tc, (void*)SDL_ThreadID());

	if(LIKELY(tls)) {
		SDL_TLSSet(tls, tc, basist_tls_destructor);
	} else {
		// this leaks; i don't care
		fallback = tc;
	}

	return tc;
}

char *texture_path(const char *name) {
	char *p = NULL;

	if((p = try_path(TEX_PATH_PREFIX, name, TEX_EXTENSION))) {
		return p;
	}

	return pixmap_source_path(TEX_PATH_PREFIX, name);
}

bool check_texture_path(const char *path) {
	if(strendswith(path, TEX_EXTENSION)) {
		return true;
	}

	return pixmap_check_filename(path);
}

static void parse_filter(const char *val, TextureFilterMode *out, bool allow_mipmaps) {
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

	log_fatal("Bad value `%s`, assuming default", pbuf);
}

static void parse_wrap(const char *val, TextureWrapMode *out) {
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

	log_warn("Bad value `%s`, assuming default", pbuf);
}

static TextureType pixmap_format_to_texture_type(PixmapFormat fmt) {
	PixmapLayout layout = PIXMAP_FORMAT_LAYOUT(fmt);
	uint depth = PIXMAP_FORMAT_DEPTH(fmt);
	bool is_float = PIXMAP_FORMAT_IS_FLOAT(fmt);

	if(is_float) {
		switch(depth) {
			case 16: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_16_FLOAT;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_16_FLOAT;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_16_FLOAT;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_16_FLOAT;
				default: UNREACHABLE;
			}

			case 32: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_32_FLOAT;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_32_FLOAT;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_32_FLOAT;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_32_FLOAT;
				default: UNREACHABLE;
			}

			default: UNREACHABLE;
		}
	} else {
		switch(depth) {
			case 8: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_8;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_8;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_8;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_8;
				default: UNREACHABLE;
			}

			case 16: switch(layout) {
				case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_16;
				case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_16;
				case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_16;
				case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_16;
				default: UNREACHABLE;
			}

			default: UNREACHABLE;
		}
	}
}

static bool parse_format(const char *val, PixmapFormat *out) {
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
		log_error("Invalid format '%s': bad channels specification, expected one of: RGBA, RGB, RG, R", val);
		return false;
	}

	assert(channels <= 4);

	char *end;
	uint depth = strtol(val + channels, &end, 10);

	if(val + channels == end) {
		log_error("Invalid format '%s': bad bit depth specification, expected an integer", val);
		return false;
	}

	if(depth != 8 && depth != 16 && depth != 32) {
		log_error("Invalid format '%s': invalid bit depth %d, only 8, 16, and 32 are supported", val, depth);
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
			log_error("Invalid format '%s': bad type specification, expected one of: U, UINT, F, FLOAT, or nothing", val);
			return false;
		}
	}

	if(depth == 32 && !is_float) {
		log_error("Invalid format '%s': bit depth %d is only supported for floating point pixels", val, depth);
		return false;
	} else if(depth < 16 && is_float) {
		log_error("Invalid format '%s': bit depth %d is not supported for floating point pixels", val, depth);
		return false;
	}

	*out = PIXMAP_MAKE_FORMAT(channels, depth) | (is_float * PIXMAP_FLOAT_BIT);
	return true;
}

typedef struct TextureLoadData {
	Pixmap pixmap;
	Pixmap pixmap_alphamap;

	uint num_mipmaps;
	Pixmap *mipmaps;

	TextureParams params;
	struct {
		// NOTE: not bitfields because we take pointers to these
		bool linearize;
		bool multiply_alpha;
		bool apply_alphamap;
	} preprocess;
} TextureLoadData;

static bool is_preprocess_needed(TextureLoadData *ld) {
	return (
		ld->preprocess.linearize |
		ld->preprocess.multiply_alpha |
		ld->preprocess.apply_alphamap |
		0
	);
}

static void dump_pixmap_format(ResourceLoadState *st, PixmapFormat fmt, const char *context) {
	log_debug("%s: %s: %d channels, %d bits per channel, %s",
		st->path, context,
		PIXMAP_FORMAT_LAYOUT(fmt),
		PIXMAP_FORMAT_DEPTH(fmt),
		PIXMAP_FORMAT_IS_FLOAT(fmt) ? "float" : "uint"
	);
}

static const char *get_basis_source_format_name(basist_source_format sfmt) {
	switch(sfmt) {
		case BASIST_SOURCE_ETC1S: return "ETC1S";
		case BASIST_SOURCE_UASTC4x4: return "UASTC";
		default: return "(unknown)";
	}
}

struct basis_size_info {
	uint32_t num_blocks;
	uint32_t block_size;
};

static struct basis_size_info get_basis_transcoded_size_info(basist_transcoder *tc, uint32_t image, uint32_t level, basist_texture_format format) {
	struct basis_size_info size_info = { 0 };

	if((uint)format >= BASIST_NUM_FORMATS) {
		log_error("Invalid format %u", format);
	}

	basist_source_format src_format = basist_transcoder_get_source_format(tc);

	if(!basist_is_format_supported(format, src_format)) {
		log_error("Can't transcode from %s into %s", get_basis_source_format_name(src_format), basist_get_format_name(format));
		return size_info;
	}

	basist_image_level_desc ldesc;

	if(!basist_transcoder_get_image_level_desc(tc, image, level, &ldesc)) {
		log_error("basist_transcoder_get_image_level_desc() failed");
		return size_info;
	}

	if(basist_is_format_uncompressed(format)) {
		size_info.block_size = basist_get_uncompressed_bytes_per_pixel(format);
		size_info.num_blocks = ldesc.orig_width * ldesc.orig_height;
		return size_info;
	}

	// NOTE: there's a caveat for PVRTC1 textures, but we don't care about them
	size_info.block_size = basist_get_bytes_per_block_or_pixel(format);
	size_info.num_blocks = ldesc.total_blocks;
	return size_info;
}

static void pack_RA8_to_RG8(Pixmap *pixmap) {
	assert(pixmap->format == PIXMAP_FORMAT_RGBA8);

	uint32_t num_pixels = pixmap->width * pixmap->height;
	PixelRG8 *rg = calloc(num_pixels, PIXMAP_FORMAT_PIXEL_SIZE(PIXMAP_FORMAT_RG8));
	PixelRGBA8 *rgba = pixmap->data.rgba8;

	#pragma omp simd safelen(32)
	for(uint32_t i = 0; i < num_pixels; ++i) {
		rg[i].r = rgba[i].r;
		rg[i].g = rgba[i].a;
	}

	free(pixmap->data.rgba8);
	pixmap->data.rg8 = rg;
	pixmap->format = PIXMAP_FORMAT_RG8;
}

#if 0
static TextureType correct_texture_type_for_sRGB(TextureType orig_type, TextureFlags tex_flags) {
	// Assumed to be handled elsewhere
	assert(!TEX_TYPE_IS_COMPRESSED(orig_type));

	TextureType type = orig_type;

	static const TextureType promotion_chain[] = {
		// NOTE: float and 16-bit textures don't really make sense for sRGB data, so they are disabled here.
		TEX_TYPE_R_8,
// 		TEX_TYPE_R_16,
// 		TEX_TYPE_R_16_FLOAT,
// 		TEX_TYPE_R_32_FLOAT,
		TEX_TYPE_RG_8,
// 		TEX_TYPE_RG_16,
// 		TEX_TYPE_RG_16_FLOAT,
// 		TEX_TYPE_RG_32_FLOAT,
		TEX_TYPE_RGB_8,
// 		TEX_TYPE_RGB_16,
// 		TEX_TYPE_RGB_16_FLOAT,
// 		TEX_TYPE_RGB_32_FLOAT,
		TEX_TYPE_RGBA_8,
// 		TEX_TYPE_RGBA_16,
// 		TEX_TYPE_RGBA_16_FLOAT,
// 		TEX_TYPE_RGBA_32_FLOAT,
	};

	bool found = false;

	for(int i = 0; i < ARRAY_SIZE(promotion_chain); ++i) {
		log_debug("iter #%i: orig_type = %s; type = %s; promotion_chain[i] = %s",
			i,
			r_texture_type_name(orig_type),
			r_texture_type_name(type),
			r_texture_type_name(promotion_chain[i])
		);
		if(promotion_chain[i] == type) {
			found = true;
			if(r_texture_type_supported(type, tex_flags | TEX_FLAG_SRGB)) {
				log_debug("Found sRGB-compatible type for texture: %s -> %s", r_texture_type_name(orig_type), r_texture_type_name(type));
				return type;
			}
		} else if(found && i < ARRAY_SIZE(promotion_chain) - 1) {
			type = promotion_chain[i + 1];
		}
	}

	log_warn("No sRGB-compatible type exists for texture type %s; converting to linear color space", r_texture_type_name(orig_type));
	return TEX_TYPE_INVALID;
}
#endif

static bool apply_uncompressed_format(TextureLoadData *ld, PixmapFormat fmt, const char *source) {
	ld->params.type = pixmap_format_to_texture_type(fmt);

	TextureTypeQueryResult qr;
	bool type_supported = r_texture_type_query(ld->params.type, ld->params.flags, ld->pixmap.format, ld->pixmap.origin, &qr);

	if(!type_supported && (ld->params.flags & TEX_FLAG_SRGB)) {
		ld->params.flags &= ~TEX_FLAG_SRGB;
		ld->preprocess.linearize = true;
		type_supported = r_texture_type_query(ld->params.type, ld->params.flags, ld->pixmap.format, ld->pixmap.origin, &qr);
	}

	if(!type_supported) {
		log_error("%s: texture type not supported", source);
		return false;
	}

	if(ld->preprocess.linearize) {
		log_warn("%s: not native sRGB support; will convert texture into linear colorspace", source);
	}

	return true;
}

static void prepare_pixmaps(Pixmap *pm_main, Pixmap *pm_alphamap, TextureType tex_type, TextureFlags tex_flags) {
	TextureTypeQueryResult qr;
	if(!r_texture_type_query(tex_type, tex_flags, pm_main->format, pm_main->origin, &qr)) {
		UNREACHABLE;
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
			UNREACHABLE;
		}

		pixmap_convert_inplace_realloc(pm_alphamap, qr.optimal_pixmap_format);
		pixmap_flip_to_origin_inplace(pm_alphamap, qr.optimal_pixmap_origin);
	}
}

static void load_texture_basisu(ResourceLoadState *st, TextureLoadData *ld, const char *source, bool is_normalmap, PixmapFormat override_format);

static void load_texture_stage1(ResourceLoadState *st) {
	const char *source = st->path;
	char *source_allocated = NULL;
	char *alphamap_allocated = NULL;

	TextureLoadData ld = {
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

			// Manual because we don't want mipmaps generated until we've applied the
			// post-load shader, which will premultiply alpha.
			// Mipmaps and filtering are basically broken without premultiplied alpha.
			.mipmap_mode = TEX_MIPMAP_MANUAL,
		},
		.preprocess.multiply_alpha = true,
	};

	PixmapFormat override_format = 0;
	bool is_normalmap = false;
	bool want_srgb = false;

	if(strendswith(st->path, TEX_EXTENSION)) {
		char *str_filter_min = NULL;
		char *str_filter_mag = NULL;
		char *str_wrap_s = NULL;
		char *str_wrap_t = NULL;
		char *str_format = NULL;

		if(!parse_keyvalue_file_with_spec(st->path, (KVSpec[]) {
			{ "source",     .out_str  = &source_allocated },
			{ "alphamap",   .out_str  = &alphamap_allocated },
			{ "filter_min", .out_str  = &str_filter_min },
			{ "filter_mag", .out_str  = &str_filter_mag },
			{ "wrap_s",     .out_str  = &str_wrap_s },
			{ "wrap_t",     .out_str  = &str_wrap_t },
			{ "format",     .out_str  = &str_format },
			{ "mipmaps",    .out_int  = (int*)&ld.params.mipmaps },
			{ "anisotropy", .out_int  = (int*)&ld.params.anisotropy },
			{ "multiply_alpha", .out_bool = &ld.preprocess.multiply_alpha },
			{ "linearize",  .out_bool = &want_srgb },
			{ "is_normalmap", .out_bool = &is_normalmap },
			{ NULL }
		})) {
			free(source_allocated);
			free(alphamap_allocated);
			res_load_failed(st);
			return;
		}

		if(!source_allocated) {
			source_allocated = pixmap_source_path(TEX_PATH_PREFIX, st->name);

			if(!source_allocated) {
				log_error("%s: couldn't infer source path from texture name", st->name);
			} else {
				log_info("%s: inferred source path from texture name: %s", st->name, source_allocated);
			}

			if(!source_allocated) {
				res_load_failed(st);
				return;
			}
		}

		source = source_allocated;

		parse_filter(str_filter_min, &ld.params.filter.min, true);
		free(str_filter_min);

		parse_filter(str_filter_mag, &ld.params.filter.mag, false);
		free(str_filter_mag);

		parse_wrap(str_wrap_s, &ld.params.wrap.s);
		free(str_wrap_s);

		parse_wrap(str_wrap_t, &ld.params.wrap.t);
		free(str_wrap_t);

		bool format_ok = parse_format(str_format, &override_format);
		free(str_format);

		if(!format_ok) {
			free(source_allocated);
			free(alphamap_allocated);
			log_error("%s: bad or unsupported pixel format specification", st->path);
			res_load_failed(st);
			return;
		}
	}

	if(want_srgb) {
		ld.params.flags |= TEX_FLAG_SRGB;
	}

	assert(!ld.preprocess.linearize);

	if(strendswith(source, ".basis")) {
		free(alphamap_allocated);
		alphamap_allocated = NULL;
		load_texture_basisu(st, &ld, source, is_normalmap, override_format);
		free(source_allocated);
		return;
	}

	if(!pixmap_load_file(source, &ld.pixmap, override_format)) {
		log_error("%s: couldn't load texture image", source);
		free(source_allocated);
		free(alphamap_allocated);
		res_load_failed(st);
		return;
	}

	if(alphamap_allocated && !pixmap_load_file(alphamap_allocated, &ld.pixmap_alphamap, PIXMAP_FORMAT_R8)) {
		log_error("%s: couldn't load texture alphamap", alphamap_allocated);
		free(ld.pixmap.data.untyped);
		free(source_allocated);
		free(alphamap_allocated);
		res_load_failed(st);
		return;
	}

	override_format = override_format ? override_format : ld.pixmap.format;
	bool apply_format_ok = apply_uncompressed_format(&ld, override_format, source);
	free(source_allocated);
	free(alphamap_allocated);

	if(!apply_format_ok) {
		free(ld.pixmap.data.untyped);
		free(ld.pixmap_alphamap.data.untyped);
		res_load_failed(st);
		return;
	}

	prepare_pixmaps(&ld.pixmap, &ld.pixmap_alphamap, ld.params.type, ld.params.flags);

	if(ld.pixmap_alphamap.data.untyped) {
		ld.preprocess.apply_alphamap = true;
	}

	if(PIXMAP_FORMAT_LAYOUT(override_format) != PIXMAP_LAYOUT_RGBA) {
		ld.preprocess.multiply_alpha = false;
	}

	if(ld.params.mipmaps == 0) {
		ld.params.mipmaps = TEX_MIPMAPS_MAX;
	}

	ld.params.width = ld.pixmap.width;
	ld.params.height = ld.pixmap.height;

	res_load_continue_on_main(st, load_texture_stage2, memdup(&ld, sizeof(ld)));
}

static void load_texture_basisu(ResourceLoadState *st, TextureLoadData *ld, const char *source, bool is_normalmap, PixmapFormat override_format) {
	basist_transcoder *tc = get_basis_transcoder();
	basist_transcode_level_params p = { 0 };
	void *filebuf = NULL;

	if(!tc) {
		goto fail;
	}

	SDL_RWops *rw_in = vfs_open(source, VFS_MODE_READ);

	if(!rw_in) {
		log_error("VFS error: %s", vfs_get_error());
		goto fail;
	}

	size_t filesize;
	filebuf = SDL_RWreadAll(rw_in, &filesize, INT32_MAX);
	SDL_RWclose(rw_in);

	if(!filebuf) {
		log_error("%s: read error: %s", source, SDL_GetError());
		goto fail;
	}

	basist_file_info file_info = { 0 };
	basist_transcoder_set_data(tc, (basist_data) { .data = filebuf, .size = filesize });

	uint32_t total_images = basist_transcoder_get_total_images(tc);

	if(total_images < 1) {
		log_error("%s: no images in texture", source);
		goto fail;
	}

	if(total_images > 1) {
		log_warn("%s: %u images in texture; only the first will be used", source, total_images);
	}

	uint32_t total_levels = basist_transcoder_get_total_image_levels(tc, 0);

	if(total_levels < 1) {
		log_error("%s: no levels in image", source);
		goto fail;
	}

	#define TRY(func, ...) \
		if(!(func(__VA_ARGS__))) { \
			log_error("%s: " #func "() failed", source); \
			goto fail; \
		}

	TRY(basist_transcoder_get_file_info, tc, &file_info);

	log_debug("Version: %u", file_info.version);
	log_debug("Header size: %u", file_info.total_header_size);
	log_debug("Source format: %u", file_info.source_format);
	log_debug("ETC1S: %u", file_info.etc1s);
	log_debug("Y-flipped: %u", file_info.y_flipped);
	log_debug("Total images: %u", file_info.total_images);
	log_debug("Total slices: %u", file_info.total_slices);

	basist_image_info image_info;
	TRY(basist_transcoder_get_image_info, tc, 0, &image_info);

	log_debug("Size: %ux%u", image_info.width, image_info.height);
	log_debug("Original size: %ux%u", image_info.orig_width, image_info.orig_height);

	basist_init_transcode_level_params(&p);
	p.format = BASIST_FORMAT_RGBA32;
	p.image_index = 0;

	ld->num_mipmaps = image_info.total_levels - 1;

	ld->params.mipmaps = image_info.total_levels;
	ld->params.mipmap_mode = TEX_MIPMAP_MANUAL;

	ld->params.width = image_info.orig_width;
	ld->params.height = image_info.orig_height;

	if(ld->num_mipmaps) {
		ld->mipmaps = calloc(ld->num_mipmaps, sizeof(*ld->mipmaps));
	} else {
		ld->mipmaps = NULL;
	}

	override_format = override_format ? override_format : PIXMAP_FORMAT_RGBA8;

	if(!apply_uncompressed_format(ld, override_format, source)) {
		goto fail;
	}

	TRY(basist_transcoder_start_transcoding, tc);

	for(uint i = 0; i < image_info.total_levels; ++i) {
		Pixmap *out_pixmap = i ? &ld->mipmaps[i - 1] : &ld->pixmap;
		p.level_index = i;

		basist_image_level_desc level_desc;
		TRY(basist_transcoder_get_image_level_desc, tc, p.image_index, p.level_index, &level_desc);

		struct basis_size_info size_info = get_basis_transcoded_size_info(tc, p.image_index, p.level_index, p.format);

		if(size_info.block_size == 0) {
			goto fail;
		}

		log_debug("Level %i [%ix%i] : %i * %i = %i", i, level_desc.orig_width, level_desc.orig_height, size_info.num_blocks, size_info.block_size, size_info.num_blocks * size_info.block_size);

		p.output_blocks = calloc(size_info.num_blocks, size_info.block_size);
		p.output_blocks_size = size_info.num_blocks;

		TRY(basist_transcoder_transcode_image_level, tc, &p);

		out_pixmap->data.untyped = p.output_blocks;
		p.output_blocks = NULL;

		out_pixmap->format = PIXMAP_FORMAT_RGBA8;
		out_pixmap->width = level_desc.orig_width;
		out_pixmap->height = level_desc.orig_height;
		out_pixmap->origin = PIXMAP_ORIGIN_TOPLEFT;

#if 0
		if(is_normalmap) {
			pack_RA8_to_RG8(&ld->pixmap);
		}
#endif

		prepare_pixmaps(out_pixmap, NULL, ld->params.type, ld->params.flags);
	}

	TRY(basist_transcoder_stop_transcoding, tc);

	if(is_normalmap) {
		// pack_RA8_to_RG8(&ld->pixmap);
		ld->params.swizzle = (TextureSwizzleMask) { "ra01" };
	}

	ld->preprocess.multiply_alpha = 0;
	ld->preprocess.apply_alphamap = 0;
// 	ld->preprocess.linearize = 0;

	res_load_continue_on_main(st, load_texture_stage2, memdup(ld, sizeof(*ld)));

cleanup:
	free(p.output_blocks);

	if(tc) {
		if(basist_transcoder_get_ready_to_transcode(tc)) {
			basist_transcoder_stop_transcoding(tc);
		}

		basist_transcoder_set_data(tc, (basist_data) { 0 });
	}

	free(filebuf);

	return;

fail:
	if(ld->mipmaps) {
		for(uint i = 0; i < ld->num_mipmaps; ++i) {
			free(ld->mipmaps[i].data.untyped);
		}

		free(ld->mipmaps);
		ld->mipmaps = NULL;
	}

	res_load_failed(st);
	goto cleanup;
}

static Texture *texture_post_load(TextureLoadData *ld, Texture *tex, Texture *alphamap) {
	// this is a bit hacky and not very efficient,
	// but it's still much faster than fixing up the texture on the CPU

	Texture *fbo_tex;
	TextureParams params;
	Framebuffer *fb;

	r_state_push();
	r_blend(BLEND_NONE);
	r_disable(RCAP_CULL_FACE);
	r_texture_get_params(tex, &params);
	params.mipmap_mode = TEX_MIPMAP_AUTO;
	r_texture_set_filter(tex, TEX_FILTER_NEAREST, TEX_FILTER_NEAREST);
	fbo_tex = r_texture_create(&params);

	r_shader("texture_post_load");
	r_uniform_sampler("tex", tex);

	r_uniform_int("linearize", ld->preprocess.linearize);
	r_uniform_int("multiply_alpha", ld->preprocess.multiply_alpha);
	r_uniform_int("apply_alphamap", ld->preprocess.apply_alphamap);
	r_uniform_int("write_srgb", (params.flags & TEX_FLAG_SRGB) == TEX_FLAG_SRGB);

	if(alphamap) {
		r_uniform_sampler("alphamap", alphamap);
	} else {
		// FIXME: even though we aren't going to actually use this sampler, it
		// must be set to something valid, lest WebGL throw a fit. This should be
		// worked around in the renderer code, not here.
		r_uniform_sampler("alphamap", tex);
	}

	r_mat_mv_push_identity();
	r_mat_proj_push_ortho(params.width, params.height);
	r_mat_mv_scale(params.width, params.height, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	fb = r_framebuffer_create();
	r_framebuffer_attach(fb, fbo_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(fb, 0, 0, params.width, params.height);
	r_framebuffer(fb);
	r_draw_quad();
	r_mat_mv_pop();
	r_mat_proj_pop();
	r_framebuffer_destroy(fb);
	r_texture_destroy(tex);
	r_state_pop();

	return fbo_tex;
}

static void load_texture_stage2(ResourceLoadState *st) {
	TextureLoadData *ld = NOT_NULL(st->opaque);
	bool preprocess_needed = is_preprocess_needed(ld);

	if(ld->mipmaps) {
		assume(!preprocess_needed);
		assume(ld->pixmap_alphamap.data.untyped == NULL);
		assert(ld->params.mipmap_mode == TEX_MIPMAP_MANUAL);
	} else if(!preprocess_needed) {
		ld->params.mipmap_mode = TEX_MIPMAP_AUTO;
	}

	Texture *texture = r_texture_create(&ld->params);

	if(preprocess_needed) {
		char namebuf[strlen(st->name) + sizeof(" (transient)")];
		snprintf(namebuf, sizeof(namebuf), "%s (transient)", st->name);
		r_texture_set_debug_label(texture, namebuf);
	} else {
		r_texture_set_debug_label(texture, st->name);
	}

	r_texture_fill(texture, 0, &ld->pixmap);
	free(ld->pixmap.data.untyped);

	for(uint i = 0; i < ld->num_mipmaps; ++i) {
		r_texture_fill(texture, i + 1, &ld->mipmaps[i]);
		free(ld->mipmaps[i].data.untyped);
	}

	free(ld->mipmaps);

	Texture *alphamap = NULL;

	if(ld->pixmap_alphamap.data.untyped) {
		TextureParams p = ld->params;
		p.type = PIXMAP_FORMAT_DEPTH(ld->pixmap_alphamap.format) > 8 ? TEX_TYPE_R_16 : TEX_TYPE_R_8;
		alphamap = r_texture_create(&p);
		const char suffix[] = " alphamap";
		char buf[strlen(st->name) + sizeof(suffix)];
		snprintf(buf, sizeof(buf), "%s%s", st->name, suffix);
		r_texture_set_debug_label(alphamap, buf);
		r_texture_fill(alphamap, 0, &ld->pixmap_alphamap);
		free(ld->pixmap_alphamap.data.untyped);
		assume(ld->preprocess.apply_alphamap);
		assume(preprocess_needed);
	}

	if(preprocess_needed) {
		texture = texture_post_load(ld, texture, alphamap);
		r_texture_set_debug_label(texture, st->name);
	}

	free(ld);

	if(alphamap) {
		r_texture_destroy(alphamap);
	}

	res_load_finished(st, texture);
}

static void free_texture(Texture *tex) {
	r_texture_destroy(tex);
}

static struct draw_texture_state {
	bool drawing;
	bool texture_matrix_tainted;
} draw_texture_state;

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex) {
	if(draw_texture_state.drawing) {
		log_fatal("Already drawing. Did you forget to call end_draw_texture, or call me on the wrong thread?");
	}

	draw_texture_state.drawing = true;

	r_uniform_sampler("tex", tex);
	r_mat_mv_push();

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	float x = dest.x;
	float y = dest.y;
	float w = dest.w;
	float h = dest.h;

	float s = frag.w/tw;
	float t = frag.h/th;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: please somehow abstract this shit away!
		frag.y = th - frag.y - frag.h;
	}

	if(s != 1 || t != 1 || frag.x || frag.y) {
		draw_texture_state.texture_matrix_tainted = true;

		r_mat_tex_push();

		r_mat_tex_scale(1.0/tw, 1.0/th, 1);

		if(frag.x || frag.y) {
			r_mat_tex_translate(frag.x, frag.y, 0);
		}

		if(s != 1 || t != 1) {
			r_mat_tex_scale(frag.w, frag.h, 1);
		}
	}

	if(x || y) {
		r_mat_mv_translate(x, y, 0);
	}

	if(w != 1 || h != 1) {
		r_mat_mv_scale(w, h, 1);
	}
}

void end_draw_texture(void) {
	assume(draw_texture_state.drawing);

	if(draw_texture_state.texture_matrix_tainted) {
		draw_texture_state.texture_matrix_tainted = false;
		r_mat_tex_pop();
	}

	r_mat_mv_pop();
	draw_texture_state.drawing = false;
}

void fill_viewport(float xoff, float yoff, float ratio, const char *name) {
	fill_viewport_p(xoff, yoff, ratio, 1, 0, res_texture(name));
}

void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex) {
	r_uniform_sampler("tex", tex);

	float rw, rh;

	if(ratio == 0) {
		rw = aspect;
		rh = 1;
	} else {
		rw = ratio * aspect;
		rh = ratio;
	}

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: we should somehow account for this globally if possible...
		yoff *= -1;
		yoff += (1 - ratio);
	}

	bool texture_matrix_tainted = false;

	if(xoff || yoff || rw != 1 || rh != 1 || angle) {
		texture_matrix_tainted = true;

		r_mat_tex_push();

		if(xoff || yoff) {
			r_mat_tex_translate(xoff, yoff, 0);
		}

		if(rw != 1 || rh != 1) {
			r_mat_tex_scale(rw, rh, 1);
		}

		if(angle) {
			r_mat_tex_translate(0.5, 0.5, 0);
			r_mat_tex_rotate(angle * DEG2RAD, 0, 0, 1);
			r_mat_tex_translate(-0.5, -0.5, 0);
		}
	}

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_W*0.5, VIEWPORT_H*0.5, 0);
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_draw_quad();
	r_mat_mv_pop();

	if(texture_matrix_tainted) {
		r_mat_tex_pop();
	}
}

void fill_screen(const char *name) {
	fill_screen_p(res_texture(name));
}

void fill_screen_p(Texture *tex) {
	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);
	begin_draw_texture((FloatRect){ SCREEN_W*0.5, SCREEN_H*0.5, SCREEN_W, SCREEN_H }, (FloatRect){ 0, 0, tw, th }, tex);
	r_draw_quad();
	end_draw_texture();
}

// draws a thin, w-width rectangle from point A to point B with a texture that
// moves along the line.
//
void loop_tex_line_p(cmplx a, cmplx b, float w, float t, Texture *texture) {
	cmplx d = b-a;
	cmplx c = (b+a)/2;

	r_mat_mv_push();
	r_mat_mv_translate(creal(c), cimag(c), 0);
	r_mat_mv_rotate(carg(d), 0, 0, 1);
	r_mat_mv_scale(cabs(d), w, 1);

	r_mat_tex_push();
	r_mat_tex_translate(t, 0, 0);

	r_uniform_sampler("tex", texture);
	r_draw_quad();

	r_mat_tex_pop();
	r_mat_mv_pop();
}

void loop_tex_line(cmplx a, cmplx b, float w, float t, const char *texture) {
	loop_tex_line_p(a, b, w, t, res_texture(texture));
}
