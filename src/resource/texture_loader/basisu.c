/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "basisu.h"
#include "basisu_cache.h"
#include "util/io.h"
#include "rwops/rwops_sha256.h"

#include <basisu_transcoder_c_api.h>

// NOTE: sha256sum + hyphen + base16 64-bit file size
#define BASISU_HASH_SIZE (SHA256_HEXDIGEST_SIZE + 17)

enum {
	BASISU_TAISEI_ID = 0x52656900,
	BASISU_TAISEI_CHANNELS_R = 0,
	BASISU_TAISEI_CHANNELS_RG = 1,
	BASISU_TAISEI_CHANNELS_RGB = 2,
	BASISU_TAISEI_CHANNELS_RGBA = 3,
	BASISU_TAISEI_CHANNELS_MASK = 0x3,
	BASISU_TAISEI_SRGB = (1 << 2),
	BASISU_TAISEI_NORMALMAP = (1 << 3),
};

struct basis_size_info {
	uint32_t num_blocks;
	uint32_t block_size;
};

#define BASIS_EXT ".basis"

char *texture_loader_basisu_try_path(const char *basename) {
	return try_path(TEX_PATH_PREFIX, basename, BASIS_EXT);
}

bool texture_loader_basisu_check_path(const char *path) {
	return strendswith(path, BASIS_EXT);
}

static void texture_loader_basisu_tls_destructor(void *tc) {
	basist_transcoder_destroy(NOT_NULL(tc));
}

static basist_transcoder *texture_loader_basisu_get_transcoder(void) {
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
		SDL_TLSSet(tls, tc, texture_loader_basisu_tls_destructor);
	} else {
		// This "leaks"; I don't care. The allocation should only happen once.
		fallback = tc;
	}

	return tc;
}

static const char *get_basis_source_format_name(basist_source_format sfmt) {
	switch(sfmt) {
		case BASIST_SOURCE_ETC1S: return "ETC1S";
		case BASIST_SOURCE_UASTC4x4: return "UASTC";
		default: return "(unknown)";
	}
}

static basist_texture_format compfmt_pixmap_to_basist(PixmapFormat fmt) {
	switch(fmt) {
		#define HANDLE(comp, layout, ...) \
		case PIXMAP_FORMAT_##comp: return BASIST_FORMAT_##comp;
		PIXMAP_COMPRESSION_FORMATS(HANDLE,)
		#undef HANDLE
		default: UNREACHABLE;
	}
}

static void pack_GA8_to_RG8(Pixmap *pixmap) {
	assert(pixmap->format == PIXMAP_FORMAT_RGBA8);

	uint32_t num_pixels = pixmap->width * pixmap->height;
	PixelRG8 *rg = calloc(num_pixels, PIXMAP_FORMAT_PIXEL_SIZE(PIXMAP_FORMAT_RG8));
	PixelRGBA8 *rgba = pixmap->data.rgba8;

	#pragma omp simd safelen(32)
	for(uint32_t i = 0; i < num_pixels; ++i) {
		rg[i].r = rgba[i].g;
		rg[i].g = rgba[i].a;
	}

	free(pixmap->data.rgba8);
	pixmap->data.rg8 = rg;
	pixmap->format = PIXMAP_FORMAT_RG8;
}

static struct basis_size_info texture_loader_basisu_get_transcoded_size_info(
	TextureLoadData *ld,
	basist_transcoder *tc,
	uint32_t image,
	uint32_t level,
	basist_texture_format format
) {
	struct basis_size_info size_info = { 0 };

	if((uint)format >= BASIST_NUM_FORMATS) {
		log_error("%s: Invalid basisu format 0x%04x", ld->st->name, format);
	}

	basist_source_format src_format = basist_transcoder_get_source_format(tc);

	if(!basist_is_format_supported(format, src_format)) {
		log_error("%s: Can't transcode from %s into %s",
			ld->tex_src_path,
			get_basis_source_format_name(src_format),
			basist_get_format_name(format)
		);
		return size_info;
	}

	basist_image_level_desc ldesc;

	if(!basist_transcoder_get_image_level_desc(tc, image, level, &ldesc)) {
		log_error("%s: basist_transcoder_get_image_level_desc() failed", ld->st->name);
		return size_info;
	}

	if(basist_is_format_uncompressed(format)) {
		size_info.block_size = basist_get_uncompressed_bytes_per_pixel(format);
		size_info.num_blocks = ldesc.orig_width * ldesc.orig_height;
	} else {
		// NOTE: there's a caveat for PVRTC1 textures, but we don't care about them
		size_info.block_size = basist_get_bytes_per_block_or_pixel(format);
		size_info.num_blocks = ldesc.total_blocks;
	}

	if(size_info.num_blocks > PIXMAP_BUFFER_MAX_SIZE / size_info.block_size) {
		log_error("%s: Image %u level %u is too large",
				  ld->st->name,
			image,
			level
		);
		memset(&size_info, 0, sizeof(size_info));
	}

	return size_info;
}

static PixmapFormat texture_loader_basisu_pick_and_apply_compressed_format(
	TextureLoadData *ld,
	uint32_t taisei_meta,
	basist_source_format basis_source_fmt,
	PixmapOrigin org,
	PixmapFormat fallback_format,
	TextureTypeQueryResult *out_qr
) {
	const char *ctx = ld->st->name;

	static int fmt_prio[4][BASIST_NUM_FORMATS] = {
		// TODO: this needs some explanation
		[BASISU_TAISEI_CHANNELS_R] = {
			PIXMAP_FORMAT_BC4_R,
			PIXMAP_FORMAT_BC5_RG,
			PIXMAP_FORMAT_BC1_RGB,
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGB,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGB,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGB,
			PIXMAP_FORMAT_FXT1_RGB,
			PIXMAP_FORMAT_ETC2_EAC_R11,
			PIXMAP_FORMAT_ETC2_EAC_RG11,
			PIXMAP_FORMAT_ETC1_RGB,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RG] = {
			PIXMAP_FORMAT_BC5_RG,
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ETC2_EAC_RG11,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RGB] = {
			PIXMAP_FORMAT_BC1_RGB,
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGB,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGB,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGB,
			PIXMAP_FORMAT_FXT1_RGB,
			PIXMAP_FORMAT_ETC1_RGB,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RGBA] = {
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
	};

	uint chan_idx = taisei_meta & BASISU_TAISEI_CHANNELS_MASK;
	assert(chan_idx < ARRAY_SIZE(fmt_prio));

	TextureTypeQueryResult qr;

	for(int *pfmt = fmt_prio[chan_idx]; *pfmt; ++pfmt) {
		PixmapFormat px_fmt = *pfmt;
		basist_texture_format basist_fmt = compfmt_pixmap_to_basist(px_fmt);

		log_debug("%s: Try %s", ctx, pixmap_format_name(px_fmt));

		if(!basist_is_format_supported(basist_fmt, basis_source_fmt)) {
			log_debug("%s: Skip: can't transcode from %s into %s",
				ctx,
				get_basis_source_format_name(basis_source_fmt),
				basist_get_format_name(basist_fmt)
			);

			continue;
		}

		TextureType tex_type = r_texture_type_from_pixmap_format(px_fmt);
		if(!texture_loader_try_set_texture_type(ld, tex_type, px_fmt, org, false, &qr)) {
			log_debug("%s: Skip: texture type %s not supported by renderer", ctx, r_texture_type_name(tex_type));
			continue;
		}

		if(!qr.supplied_pixmap_origin_supported) {
			log_debug("%s: Skip: supplied origin not supported; flipping not implemented", ctx);
			continue;
		}

		assert(qr.supplied_pixmap_format_supported);

		ld->params.type = tex_type;

		if(out_qr) {
			*out_qr = qr;
		}

		return px_fmt;
	}

	log_warn("%s: No suitable compression format available; falling back to uncompressed RGBA", ctx);

	TextureType tex_type = r_texture_type_from_pixmap_format(fallback_format);
	if(texture_loader_set_texture_type_uncompressed(ld, tex_type, PIXMAP_FORMAT_RGBA8, org, &qr)) {
		if(out_qr) {
			*out_qr = qr;
		}

		assert(qr.supplied_pixmap_format_supported);
		assert(qr.supplied_pixmap_origin_supported);

		return qr.optimal_pixmap_format;
	}

	return 0;
}

struct basisu_load_data {
	char *filebuf;
	basist_transcoder *tc;
};

static void texture_loader_basisu_cleanup(struct basisu_load_data *bld) {
	if(bld->tc) {
		if(basist_transcoder_get_ready_to_transcode(bld->tc)) {
			basist_transcoder_stop_transcoding(bld->tc);
		}

		basist_transcoder_set_data(bld->tc, (basist_data) { 0 });
	}

	free(bld->filebuf);
	bld->filebuf = NULL;
}

static void texture_loader_basisu_failed(TextureLoadData *ld, struct basisu_load_data *bld) {
	texture_loader_basisu_cleanup(bld);
	texture_loader_failed(ld);
}

static char *read_basis_file(SDL_RWops *rw, size_t *file_size, size_t hash_size, char hash[hash_size]) {
	assert(hash_size >= BASISU_HASH_SIZE);

	SHA256State *sha256 = sha256_new();
	rw = SDL_RWWrapSHA256(rw, sha256, false);
	char *buf = SDL_RWreadAll(rw, file_size, INT32_MAX);
	SDL_RWclose(rw);

	uint8_t raw_hash[SHA256_BLOCK_SIZE];
	sha256_final(sha256, raw_hash, sizeof(raw_hash));
	sha256_free(sha256);
	hexdigest(raw_hash, sizeof(raw_hash), hash, hash_size);

	assert(hash[SHA256_HEXDIGEST_SIZE - 1] == 0);
	snprintf(&hash[SHA256_HEXDIGEST_SIZE - 1], BASISU_HASH_SIZE - SHA256_HEXDIGEST_SIZE, "-%zx", *file_size);

	return buf;
}

void texture_loader_basisu(TextureLoadData *ld) {
	struct basisu_load_data bld = { 0 };

	if(!(bld.tc = texture_loader_basisu_get_transcoder())) {
		texture_loader_basisu_failed(ld, &bld);
	}

	const char *ctx = ld->st->name;
	const char *basis_file = ld->tex_src_path;

	SDL_RWops *rw_in = vfs_open(basis_file, VFS_MODE_READ);
	if(!rw_in) {
		log_error("%s: VFS error: %s", ctx, vfs_get_error());
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	size_t filesize;
	char basis_hash[BASISU_HASH_SIZE];
	bld.filebuf = read_basis_file(rw_in, &filesize, sizeof(basis_hash), basis_hash);
	SDL_RWclose(rw_in);

	if(!bld.filebuf) {
		log_error("%s: Read error: %s", ld->tex_src_path, SDL_GetError());
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	#define TRY(func, ...) \
		if(!(func(__VA_ARGS__))) { \
			log_error("%s: " #func "() failed", ctx); \
			texture_loader_basisu_failed(ld, &bld); \
			return; \
		}

	assert(!basist_transcoder_get_ready_to_transcode(bld.tc));

	basist_transcoder_set_data(bld.tc, (basist_data) { .data = bld.filebuf, .size = filesize });
	log_info("%s: Loaded Basis Universal data from %s", ctx, basis_file);

	basist_file_info file_info = { 0 };
	TRY(basist_transcoder_get_file_info, bld.tc, &file_info);

	log_debug("Version: %u", file_info.version);
	log_debug("Header size: %u", file_info.total_header_size);
	log_debug("Source format: %s", get_basis_source_format_name(file_info.source_format));
	log_debug("Y-flipped: %u", file_info.y_flipped);
	log_debug("Total images: %u", file_info.total_images);
	log_debug("Total slices: %u", file_info.total_slices);
	log_debug("Has alpha slices: %u", file_info.has_alpha_slices);
	log_debug("Userdata0: 0x%08x", file_info.userdata.userdata[0]);
	log_debug("Userdata1: 0x%08x", file_info.userdata.userdata[1]);

	if(file_info.tex_type != BASIST_TYPE_2D) {
		log_error("%s: Unsupported Basis Universal texture type %s",
			ctx,
			basist_get_texture_type_name(file_info.tex_type)
		);
	}

	if(file_info.total_images < 1) {
		log_error("%s: No images in Basis Universal texture", ctx);
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	uint32_t image_idx = 0;

	if(file_info.total_images > 1) {
		log_warn("%s: Basis Universal texture contains more than 1 image; only image %u will be used", ctx, image_idx);
	}

	basist_image_info image_info;
	TRY(basist_transcoder_get_image_info, bld.tc, image_idx, &image_info);

	log_debug("Size: %ux%u", image_info.width, image_info.height);
	log_debug("Original size: %ux%u", image_info.orig_width, image_info.orig_height);

	if(image_info.total_levels < 1) {
		log_error("%s: No levels in image %u in Basis Universal texture", ctx, image_idx);
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	uint32_t taisei_meta = file_info.userdata.userdata[0];

	if((file_info.userdata.userdata[1] & 0xFFFFFF00) != BASISU_TAISEI_ID) {
		log_warn("%s: No Taisei metadata in Basis Universal texture", ctx);

		if(file_info.has_alpha_slices) {
			taisei_meta = BASISU_TAISEI_CHANNELS_RGBA;
		} else {
			taisei_meta = BASISU_TAISEI_CHANNELS_RGB;
		}

		assert(0);
	}

	if(taisei_meta & BASISU_TAISEI_SRGB) {
		ld->params.flags |= TEX_FLAG_SRGB;
	} else {
		ld->params.flags &= ~TEX_FLAG_SRGB;
	}

	PixmapOrigin px_origin = file_info.y_flipped ? PIXMAP_ORIGIN_BOTTOMLEFT : PIXMAP_ORIGIN_TOPLEFT;
	PixmapFormat px_fallback_format = PIXMAP_FORMAT_RGBA8;
	basist_texture_format basis_fallback_format = BASIST_FORMAT_RGBA32;

	TextureTypeQueryResult qr = { 0 };

	PixmapFormat choosen_format = texture_loader_basisu_pick_and_apply_compressed_format(
		ld,
		taisei_meta,
		file_info.source_format,
		px_origin,
		ld->preferred_format ? ld->preferred_format : px_fallback_format,
		&qr
	);

	if(!choosen_format) {
		log_error("%s: Could not choose texture type", ctx);
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	basist_transcode_level_params p = { 0 };
	basist_init_transcode_level_params(&p);
	p.image_index = image_idx;

	PixmapFormat px_decode_format;
	bool is_uncompressed_fallback;

	if(pixmap_format_is_compressed(choosen_format)) {
		px_decode_format = choosen_format;
		p.format = compfmt_pixmap_to_basist(choosen_format);
		is_uncompressed_fallback = false;
	} else {
		px_decode_format = px_fallback_format;
		p.format = basis_fallback_format;
		is_uncompressed_fallback = true;
	}

	int mip_bias = env_get_int("TAISEI_BASISU_MIP_BIAS", 0);
	mip_bias = iclamp(mip_bias, 0, image_info.total_levels - 1);

	// NOTE: the 0th mip level is stored in ld->pixmap
	// ld->mipmaps and ld->num_mipmaps are for extra mip levels
	ld->num_mipmaps = image_info.total_levels - 1 - mip_bias;

	// But in TextureParams, the mipmap count includes the 0th level
	ld->params.mipmaps = image_info.total_levels - mip_bias;
	ld->params.mipmap_mode = TEX_MIPMAP_MANUAL;

	// ld->params.width = image_info.orig_width;
	// ld->params.height = image_info.orig_height;

	if(ld->num_mipmaps) {
		ld->mipmaps = calloc(ld->num_mipmaps, sizeof(*ld->mipmaps));
	} else {
		ld->mipmaps = NULL;
	}

	bool transcoding_started = false;

	for(uint i = mip_bias; i < image_info.total_levels; ++i) {
		p.level_index = i;

		basist_image_level_desc level_desc;
		TRY(basist_transcoder_get_image_level_desc, bld.tc, p.image_index, p.level_index, &level_desc);

		Pixmap *out_pixmap = NULL;

		if(i > mip_bias) {
			out_pixmap = &ld->mipmaps[i - 1 - mip_bias];
		} else {
			out_pixmap = &ld->pixmap;
			ld->params.width = level_desc.orig_width;
			ld->params.height = level_desc.orig_height;
		}

		struct basis_size_info size_info = texture_loader_basisu_get_transcoded_size_info(
			ld, bld.tc, p.image_index, p.level_index, p.format
		);

		if(size_info.block_size == 0) {
			texture_loader_basisu_failed(ld, &bld);
			return;
		}

		log_debug("Level %i [%ix%i] : %i * %i = %i",
			i,
			level_desc.orig_width, level_desc.orig_height,
			size_info.num_blocks, size_info.block_size,
			size_info.num_blocks * size_info.block_size
		);

		uint32_t data_size = size_info.num_blocks * size_info.block_size;

		if(!texture_loader_basisu_load_cached(
			basis_hash,
			&p,
			&level_desc,
			px_decode_format,
			px_origin,
			data_size,
			out_pixmap
		)) {
			if(!transcoding_started) {
				TRY(basist_transcoder_start_transcoding, bld.tc);
				transcoding_started = true;
			}

			out_pixmap->data_size = data_size;
			out_pixmap->data.untyped = calloc(1, out_pixmap->data_size);
			p.output_blocks = out_pixmap->data.untyped;
			p.output_blocks_size = size_info.num_blocks;

			TRY(basist_transcoder_transcode_image_level, bld.tc, &p);

			out_pixmap->format = px_decode_format;
			out_pixmap->width = level_desc.orig_width;
			out_pixmap->height = level_desc.orig_height;
			out_pixmap->origin = px_origin;

			texture_loader_basisu_cache(basis_hash, &p, &level_desc, out_pixmap);
		}

		if(is_uncompressed_fallback) {
			// NOTE: it doesn't hurt to call this in the compressed case as well,
			// but it's effectively a no-op with a redundant texture type query.

			if(!texture_loader_prepare_pixmaps(ld, out_pixmap, NULL, ld->params.type, ld->params.flags)) {
				texture_loader_basisu_failed(ld, &bld);
				return;
			}
		}
	}

	TRY(basist_transcoder_stop_transcoding, bld.tc);

	PixmapLayout channels = pixmap_format_layout(ld->pixmap.format);

	switch(taisei_meta & BASISU_TAISEI_CHANNELS_MASK) {
		// NOTE: Using the green channel for monochrome data ensures best precision,
		// because color endpoints are stored as RGB565 in some formats.

		case BASISU_TAISEI_CHANNELS_R:
			if(channels == PIXMAP_LAYOUT_R) {
				ld->params.swizzle = (TextureSwizzleMask) { "rrr1" };
			} else {
				ld->params.swizzle = (TextureSwizzleMask) { "ggg1" };
			}
			break;

		case BASISU_TAISEI_CHANNELS_RG:
			if(channels == PIXMAP_LAYOUT_RGBA) {
				ld->params.swizzle = (TextureSwizzleMask) { "ga01" };
			} else {
				assert(channels == PIXMAP_LAYOUT_RG);
			}
			break;

		case BASISU_TAISEI_CHANNELS_RGB:
			if(channels == PIXMAP_LAYOUT_RGBA) {
				ld->params.swizzle = (TextureSwizzleMask) { "rgb1" };
			} else {
				assert(channels == PIXMAP_LAYOUT_RGB);
			}
			break;

		case BASISU_TAISEI_CHANNELS_RGBA:
			break;

		default:
			UNREACHABLE;
	}

	// These are expected to be pre-applied if needed
	ld->preprocess.multiply_alpha = 0;
	ld->preprocess.apply_alphamap = 0;

	// Don't unset this one; it may be set in case we've fallen back to decompression
	// and the renderer has no sRGB sampling support for this texture type.
	// ld->preprocess.linearize = 0;

	texture_loader_basisu_cleanup(&bld);
	texture_loader_continue(ld);
}
