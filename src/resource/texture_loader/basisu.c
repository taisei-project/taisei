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
	BASISU_TAISEI_ID            = 0x52656900,
	BASISU_TAISEI_CHANNELS_R    = 0,
	BASISU_TAISEI_CHANNELS_RG   = 1,
	BASISU_TAISEI_CHANNELS_RGB  = 2,
	BASISU_TAISEI_CHANNELS_RGBA = 3,
	BASISU_TAISEI_CHANNELS_MASK = 0x3,
	BASISU_TAISEI_SRGB          = (BASISU_TAISEI_CHANNELS_MASK + 1) << 0,
	BASISU_TAISEI_NORMALMAP     = (BASISU_TAISEI_CHANNELS_MASK + 1) << 1,
	BASISU_TAISEI_GRAYALPHA     = (BASISU_TAISEI_CHANNELS_MASK + 1) << 2,
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

	BASISU_DEBUG("Created Basis Universal transcoder %p for thread %p", (void*)tc, (void*)SDL_ThreadID());

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
			ld->src_paths.main,
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
	TextureTypeQueryResult qr;

	if(env_get("TAISEI_BASISU_FORCE_UNCOMPRESSED", false)) {
		log_info("%s: Uncompressed fallback forced by environment", ctx);
		goto uncompressed;
	}

	static int fmt_prio[4][BASIST_NUM_FORMATS] = {
		/*
		 * NOTE: Some key considerations:
		 *
		 *   - This priority assumes source textures are in ETC1S format, not UASTC.
		 *
		 *   - ETC formats are the fastest to transcode to, because ETC1S is essentially a ETC1/ETC2 subset.
		 *
		 *   - ETC formats are usually not supported by desktop GPUs.
		 *
		 *   - Desktop OpenGL usually exposes ETC support as part of the ARB_ES3_compatibility extension.
		 *
		 *   - ETC on desktop is usually emulated as an uncompressed format. Uploading ETC textures in this case is
		 *     INCREDIBLY SLOW, at least in Mesa, because the GL driver is forced to decompress them on the main thread,
		 *     sequentially. This completely negates the transcoding speed advantage of ETC.
		 *
		 *   - There is no reliable way in OpenGL to query whether a compressed format is emulated or not.
		 *
		 *   - ASTC is also rarely available on the desktop. It's required by ES 3.2, but not by ARB_ES3_2_compatibility.
		 *     Despite this, ASTC support is still often emulated on desktop GL. However, ASTC uploads appear to be much
		 *     faster than ETC ones (on Mesa).
		 *
		 *   - S3TC (which covers BC1, BC3, BC4, BC5) is near-universal on desktop and almost-nonexistent on mobile. BC7
		 *     (also known as BPTC) is not as widespread, but the situation is similar.
		 *
		 *   - BC7/BPTC preserves quality better than BC1 and BC3, at the cost of being a bit larger.
		 *
		 *   - Despite emulated ASTC being much faster to upload on desktop, I've placed it below ETC in the priority
		 *     lists. This is because on the desktop, one of the BC* formats will be picked 99% of the time anyway, and
		 *     if not, then ASTC is unlikely to be available either. On mobile, ETC has an advantage because of its fast  *      *     transcode speed, lower bitrate, and lossless transcoding.
		 *
		 *   - ATC and FXT1 are old and obscure. ATC appears to have some presence on mobile. FXT1 is effectively Intel-
		 *     only. Both are not expected to be emulated.
		 *
		 *   - PVRTC is mostly limited to Apple's mobile devices, and also is not expected to be emulated.
		 *
		 *   - PVRTC1 requires textures to be power-of-two quads, is slow to transcode to, and is generally inferior to
		 *     PVRTC2.
		 *
		 *   - I have been unable to test PVRTC1, PVRTC2, ATC, and FXT1 so far.
		 */

		[BASISU_TAISEI_CHANNELS_R] = {
			PIXMAP_FORMAT_BC4_R,
			PIXMAP_FORMAT_BC5_RG,
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_BC1_RGB,
			PIXMAP_FORMAT_BC3_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGB,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGB,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGB,
			PIXMAP_FORMAT_ATC_RGBA,
			PIXMAP_FORMAT_FXT1_RGB,
			PIXMAP_FORMAT_ETC2_EAC_R11,
			PIXMAP_FORMAT_ETC2_EAC_RG11,
			PIXMAP_FORMAT_ETC1_RGB,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RG] = {
			// NOTE: most (all?) RGB formats are unsuitable for normalmaps/non-color data due to "crosstalk" between the
			// channels. RG formats are specifically meant for this, but RGBA formats also work. In the RGBA case we
			// use the color part for the R channel and the alpha part for the G channel, and use a swizzle mask to
			// transparently correct sampling.
			PIXMAP_FORMAT_BC5_RG,
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_BC3_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGBA,
			PIXMAP_FORMAT_ETC2_EAC_RG11,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RGB] = {
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_BC1_RGB,
			PIXMAP_FORMAT_BC3_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGB,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGB,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGB,
			PIXMAP_FORMAT_ATC_RGBA,
			PIXMAP_FORMAT_FXT1_RGB,
			PIXMAP_FORMAT_ETC1_RGB,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
		[BASISU_TAISEI_CHANNELS_RGBA] = {
			PIXMAP_FORMAT_BC7_RGBA,
			PIXMAP_FORMAT_BC3_RGBA,
			PIXMAP_FORMAT_PVRTC2_4_RGBA,
			// PIXMAP_FORMAT_PVRTC1_4_RGBA,
			PIXMAP_FORMAT_ATC_RGBA,
			PIXMAP_FORMAT_ETC2_RGBA,
			PIXMAP_FORMAT_ASTC_4x4_RGBA,
			0,
		},
	};

	uint chan_idx = taisei_meta & BASISU_TAISEI_CHANNELS_MASK;
	assert(chan_idx < ARRAY_SIZE(fmt_prio));

	bool swizzle_supported = r_supports(RFEAT_TEXTURE_SWIZZLE);

	for(int *pfmt = fmt_prio[chan_idx]; *pfmt; ++pfmt) {
		PixmapFormat px_fmt = *pfmt;
		basist_texture_format basist_fmt = compfmt_pixmap_to_basist(px_fmt);

		BASISU_DEBUG("%s: Try %s", ctx, pixmap_format_name(px_fmt));

		if(!basist_is_format_supported(basist_fmt, basis_source_fmt)) {
			BASISU_DEBUG("%s: Skip: can't transcode from %s into %s",
				ctx,
				get_basis_source_format_name(basis_source_fmt),
				basist_get_format_name(basist_fmt)
			);

			continue;
		}

		if(chan_idx == BASISU_TAISEI_CHANNELS_RG && !swizzle_supported) {
			uint nchans = pixmap_format_layout(px_fmt);

			if(taisei_meta & BASISU_TAISEI_GRAYALPHA) {
				if(nchans == 2) {
					BASISU_DEBUG("%s: Skip: can't swizzle RG01 into RRRG", ctx);
					continue;
				}
			} else {
				if(nchans == 4) {
					BASISU_DEBUG("%s: Skip: can't swizzle RGBA into GA01", ctx);
					continue;
				}
			}
		}

		TextureType tex_type = r_texture_type_from_pixmap_format(px_fmt);
		if(!texture_loader_try_set_texture_type(ld, tex_type, px_fmt, org, false, &qr)) {
			BASISU_DEBUG("%s: Skip: texture type %s not supported by renderer", ctx, r_texture_type_name(tex_type));
			continue;
		}

		if(!qr.supplied_pixmap_origin_supported) {
			BASISU_DEBUG("%s: Skip: supplied origin not supported; flipping not implemented", ctx);
			continue;
		}

		assert(qr.supplied_pixmap_format_supported);

		ld->params.type = tex_type;

		if(out_qr) {
			*out_qr = qr;
		}

		log_info("%s: Transcoding into %s", ctx, basist_get_format_name(basist_fmt));
		return px_fmt;
	}

	log_warn("%s: No suitable compression format available; falling back to uncompressed RGBA", ctx);

uncompressed:
	(void)0;  // C is dumb

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
	uint mip_bias;
	PixmapFormat px_decode_format;
	PixmapOrigin px_origin;
	bool transcoding_started;
	bool swizzle_supported;
	bool is_uncompressed_fallback;
	char basis_hash[BASISU_HASH_SIZE];
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
	char *buf = NULL;

	if(LIKELY(rw)) {
		buf = SDL_RWreadAll(rw, file_size, INT32_MAX);
		SDL_RWclose(rw);
	}

	if(UNLIKELY(!buf)) {
		*file_size = 0;
		sha256_free(sha256);
		return NULL;
	}

	uint8_t raw_hash[SHA256_BLOCK_SIZE];
	sha256_final(sha256, raw_hash, sizeof(raw_hash));
	sha256_free(sha256);
	hexdigest(raw_hash, sizeof(raw_hash), hash, hash_size);

	assert(hash[SHA256_HEXDIGEST_SIZE - 1] == 0);
	snprintf(&hash[SHA256_HEXDIGEST_SIZE - 1], BASISU_HASH_SIZE - SHA256_HEXDIGEST_SIZE, "-%zx", *file_size);

	return buf;
}

static void texture_loader_basisu_set_swizzle(TextureLoadData *ld, PixmapFormat fmt, uint32_t taisei_meta) {
	PixmapLayout channels = pixmap_format_layout(fmt);

	switch(taisei_meta & BASISU_TAISEI_CHANNELS_MASK) {
		// NOTE: Using the green channel for monochrome data ensures best precision,
		// because color endpoints are stored as RGB565 in some formats.

		case BASISU_TAISEI_CHANNELS_R:
			if(channels == PIXMAP_LAYOUT_R) {
				ld->params.swizzle = (SwizzleMask) { "rrr1" };
			} else {
				ld->params.swizzle = (SwizzleMask) { "ggg1" };
			}
			break;

		case BASISU_TAISEI_CHANNELS_RG:
			if(channels == PIXMAP_LAYOUT_RGBA) {
				if(taisei_meta & BASISU_TAISEI_GRAYALPHA) {
					ld->params.swizzle = (SwizzleMask) { "ggga" };
				} else {
					ld->params.swizzle = (SwizzleMask) { "ga01" };
				}
			} else {
				assert(channels == PIXMAP_LAYOUT_RG);
				if(taisei_meta & BASISU_TAISEI_GRAYALPHA) {
					ld->params.swizzle = (SwizzleMask) { "rrrg" };
				}
			}
			break;

		case BASISU_TAISEI_CHANNELS_RGB:
			if(channels == PIXMAP_LAYOUT_RGBA) {
				ld->params.swizzle = (SwizzleMask) { "rgb1" };
			} else {
				assert(channels == PIXMAP_LAYOUT_RGB);
			}
			break;

		case BASISU_TAISEI_CHANNELS_RGBA:
			break;

		default:
			UNREACHABLE;
	}
}

#define TRY(func, ...) \
	if(UNLIKELY(!(func(__VA_ARGS__)))) { \
		log_error("%s: " #func "() failed", ctx); \
		texture_loader_basisu_failed(ld, &bld); \
		return; \
	}

#define TRY_SILENT(func, ...) \
	if(UNLIKELY(!(func(__VA_ARGS__)))) { \
		texture_loader_basisu_failed(ld, &bld); \
		return; \
	}

#define TRY_BOOL(func, ...) \
	if(UNLIKELY(!(func(__VA_ARGS__)))) { \
		log_error("%s: " #func "() failed", ctx); \
		return false; \
	}

static bool texture_loader_basisu_check_consistency(
	const char *ctx,
	basist_transcoder *tc,
	basist_file_info *file_info,
	basist_image_info img_infos[]
) {
	basist_image_info *ref = img_infos;

	if(ref->total_levels < 1) {
		log_error("%s: No levels in image 0 in Basis Universal texture", ctx);
		return false;
	}

	bool ok = true;

	for(uint i = 1; i < file_info->total_images; ++i) {
		basist_image_info *img = img_infos + i;

		if(img->width != ref->width || img->height != ref->height) {
			log_error(
				"%s: Image %i size is different from image 0 (%ux%u != %ux%u); "
				"this is not allowed",
				ctx, i, img->width, img->height, ref->width, ref->height
			);
			ok = false;
		}

		if(img->total_levels != ref->total_levels) {
			log_error(
				"%s: Image %i has different number of mip levels from image 0 (%u != %u); "
				"this is not allowed",
				ctx, i, img->total_levels, ref->total_levels
			);
			ok = false;
		}

		// TODO: check if level dimensions are sane?
	}

	return ok;
}

static bool texture_loader_basisu_sanitize_levels(
	const char *ctx,
	basist_transcoder *tc,
	const basist_image_info *image_info,
	uint *out_total_levels
) {
	/*
	 * NOTE: We assume the renderer wants all mip levels of compressed textures to have
	 * dimensions divisible by 4 (if dimension > 2). This is true for most (all?) BC[1-7]
	 * implementations in GLES, but other formats and/or renderers may have more lax requirements
	 * (core GL with ARB_texture_compression_{s3tc,bptc} in particular), or even stricter
	 * requirements.
	 *
	 * TODO: Add a renderer API to query such requirements on a per-format basis, and use it
	 * here.
	 */

	for(uint i = 1; i < image_info->total_levels; ++i) {
		basist_image_level_desc level_desc;
		TRY_BOOL(
			basist_transcoder_get_image_level_desc, tc, image_info->image_index, i, &level_desc
		);

		if(
			(level_desc.orig_width  > 2 && level_desc.orig_width  % 4) ||
			(level_desc.orig_height > 2 && level_desc.orig_height % 4)
		) {
			log_warn(
				"%s: Mip level %i dimensions are not multiples of 4 (%ix%i); "
				"number of levels reduced %i -> %i",
				ctx, i, level_desc.orig_width, level_desc.orig_height, image_info->total_levels, i
			);
			*out_total_levels = i;
			return true;
		}
	}

	*out_total_levels = image_info->total_levels;
	return true;
}

static bool texture_loader_basisu_init_mipmaps(
	const char *ctx,
	struct basisu_load_data *bld,
	TextureLoadData *ld,
	basist_image_info *ref_img
) {
	uint total_levels;

	if(bld->is_uncompressed_fallback) {
		total_levels = ref_img->total_levels;
	} else {
		TRY_BOOL(texture_loader_basisu_sanitize_levels, ctx, bld->tc, ref_img, &total_levels);
	}

	int mip_bias = env_get("TAISEI_BASISU_MIP_BIAS", 0);
	mip_bias = iclamp(mip_bias, 0, total_levels - 1);
	{
		int max_levels = env_get("TAISEI_BASISU_MAX_MIP_LEVELS", 0);
		if(max_levels > 0) {
			total_levels = iclamp(total_levels, 1, mip_bias + max_levels);
		}
	}
	bld->mip_bias = mip_bias;

	ld->params.mipmaps = total_levels - mip_bias;
	ld->params.mipmap_mode = TEX_MIPMAP_MANUAL;

	basist_image_level_desc zero_level;
	TRY_BOOL(
		basist_transcoder_get_image_level_desc,
		bld->tc, ref_img->image_index, mip_bias, &zero_level
	);

	uint max_mips = r_texture_util_max_num_miplevels(zero_level.orig_width, zero_level.orig_height);

	if(
		ld->params.mipmaps > 1 &&
		ld->params.mipmaps < max_mips &&
		!r_supports(RFEAT_PARTIAL_MIPMAPS)
	) {
		log_warn(
			"%s: Texture has partial mipmaps (%i levels out of %i), "
			"but the renderer doesn't support this. "
			"Mipmapping will be disabled for this texture",
			ctx, ld->params.mipmaps, max_mips
		);

		ld->params.mipmaps = 1;

		// TODO: In case of decompression fallback,
		// we could switch to auto-generated mipmaps here instead.
		// Untested code follows:
#if 0
		if(bld->is_uncompressed_fallback) {
			ld->params.mipmaps = max_mips;
			ld->params.mipmap_mode = TEX_MIPMAP_AUTO;
		}
#endif
	}

	switch(ld->params.class) {
		case TEXTURE_CLASS_2D:
			ld->pixmaps = calloc(ld->params.mipmaps, sizeof(*ld->pixmaps));
			ld->num_pixmaps = ld->params.mipmaps;
			break;

		case TEXTURE_CLASS_CUBEMAP:
			ld->cubemaps = calloc(ld->params.mipmaps, sizeof(*ld->cubemaps));
			ld->num_pixmaps = ld->params.mipmaps * 6;
			break;

		default: UNREACHABLE;
	}

	ld->params.width = zero_level.orig_width;
	ld->params.height = zero_level.orig_height;

	return true;
}

static bool texture_loader_basisu_load_pixmap(
	const char *ctx,
	struct basisu_load_data *bld,
	TextureLoadData *ld,
	basist_transcode_level_params *parm,
	Pixmap *out_pixmap
) {
	basist_image_level_desc level_desc;
	TRY_BOOL(
		basist_transcoder_get_image_level_desc,
		bld->tc, parm->image_index, parm->level_index, &level_desc
	);

	struct basis_size_info size_info = texture_loader_basisu_get_transcoded_size_info(
		ld, bld->tc, parm->image_index, parm->level_index, parm->format
	);

	if(size_info.block_size == 0) {
		return false;
	}

	BASISU_DEBUG("Image %i  Level %i   [%ix%i] : %i * %i = %i",
		parm->image_index, parm->level_index,
		level_desc.orig_width, level_desc.orig_height,
		size_info.num_blocks, size_info.block_size,
		size_info.num_blocks * size_info.block_size
	);

	uint32_t data_size = size_info.num_blocks * size_info.block_size;

	if(!texture_loader_basisu_load_cached(
		bld->basis_hash,
		parm,
		&level_desc,
		bld->px_decode_format,
		bld->px_origin,
		data_size,
		out_pixmap
	)) {
		if(!bld->transcoding_started) {
			TRY_BOOL(basist_transcoder_start_transcoding, bld->tc);
			bld->transcoding_started = true;
		}

		out_pixmap->data_size = data_size;
		out_pixmap->data.untyped = calloc(1, out_pixmap->data_size);
		parm->output_blocks = out_pixmap->data.untyped;
		parm->output_blocks_size = size_info.num_blocks;

		TRY_BOOL(basist_transcoder_transcode_image_level, bld->tc, parm);

		out_pixmap->format = bld->px_decode_format;
		out_pixmap->width = level_desc.orig_width;
		out_pixmap->height = level_desc.orig_height;
		out_pixmap->origin = bld->px_origin;

		texture_loader_basisu_cache(bld->basis_hash, parm, &level_desc, out_pixmap);
	}

	if(bld->is_uncompressed_fallback) {
		// TODO: maybe cache the swizzle result?
		if(!bld->swizzle_supported) {
			pixmap_swizzle_inplace(out_pixmap, ld->params.swizzle);
		}

		// NOTE: it doesn't hurt to call this in the compressed case as well,
		// but it's effectively a no-op with a redundant texture type query.

		if(!texture_loader_prepare_pixmaps(
			ld, out_pixmap, NULL, ld->params.type, ld->params.flags
		)) {
			return false;
		}
	}

	return true;
}

void texture_loader_basisu(TextureLoadData *ld) {
	struct basisu_load_data bld = { 0 };

	if(UNLIKELY(!(bld.tc = texture_loader_basisu_get_transcoder()))) {
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	const char *ctx = ld->st->name;
	const char *basis_file = ld->src_paths.main;

	SDL_RWops *rw_in = vfs_open(basis_file, VFS_MODE_READ);
	if(!UNLIKELY(rw_in)) {
		log_error("%s: VFS error: %s", ctx, vfs_get_error());
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	size_t filesize;
	bld.filebuf = read_basis_file(rw_in, &filesize, sizeof(bld.basis_hash), bld.basis_hash);
	SDL_RWclose(rw_in);

	if(UNLIKELY(!bld.filebuf)) {
		log_error("%s: Read error: %s", basis_file, SDL_GetError());
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	assert(!basist_transcoder_get_ready_to_transcode(bld.tc));

	basist_transcoder_set_data(bld.tc, (basist_data) { .data = bld.filebuf, .size = filesize });
	log_info("%s: Loaded Basis Universal data from %s", ctx, basis_file);

	basist_file_info file_info = { 0 };
	TRY(basist_transcoder_get_file_info, bld.tc, &file_info);

	BASISU_DEBUG("Version: %u", file_info.version);
	BASISU_DEBUG("Header size: %u", file_info.total_header_size);
	BASISU_DEBUG("Source format: %s", get_basis_source_format_name(file_info.source_format));
	BASISU_DEBUG("Y-flipped: %u", file_info.y_flipped);
	BASISU_DEBUG("Total images: %u", file_info.total_images);
	BASISU_DEBUG("Total slices: %u", file_info.total_slices);
	BASISU_DEBUG("Has alpha slices: %u", file_info.has_alpha_slices);
	BASISU_DEBUG("Userdata0: 0x%08x", file_info.userdata.userdata[0]);
	BASISU_DEBUG("Userdata1: 0x%08x", file_info.userdata.userdata[1]);

	if(file_info.total_images < 1) {
		log_error("%s: No images in Basis Universal texture", ctx);
		texture_loader_basisu_failed(ld, &bld);
		return;
	}

	uint num_load_images;

	switch(file_info.tex_type) {
		case BASIST_TYPE_2D:
			num_load_images = 1;

			if(file_info.total_images > num_load_images) {
				log_warn("%s: Basis Universal texture contains more than 1 image; only image 0 will be used", ctx);
			}

			ld->params.class = TEXTURE_CLASS_2D;
			break;

		case BASIST_TYPE_CUBEMAP_ARRAY:
			num_load_images = 6;

			if(file_info.total_images < num_load_images) {
				log_error("%s: Cubemap contains only %u faces; need 6",
					ctx, file_info.total_images
				);
				texture_loader_basisu_failed(ld, &bld);
				return;
			}

			if(file_info.total_images > num_load_images) {
				log_warn("%s: Basis Universal cubemap contains more than 6 faces; only first 6 will be used", ctx);
			}

			ld->params.class = TEXTURE_CLASS_CUBEMAP;
			break;

		default:
			log_error("%s: Unsupported Basis Universal texture type %s",
				ctx, basist_get_texture_type_name(file_info.tex_type)
			);
			texture_loader_basisu_failed(ld, &bld);
			return;
	}

	file_info.total_images = num_load_images;

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

	bld.px_origin = file_info.y_flipped ? PIXMAP_ORIGIN_BOTTOMLEFT : PIXMAP_ORIGIN_TOPLEFT;
	PixmapFormat px_fallback_format = PIXMAP_FORMAT_RGBA8;
	basist_texture_format basis_fallback_format = BASIST_FORMAT_RGBA32;

	TextureTypeQueryResult qr = { 0 };

	PixmapFormat choosen_format = texture_loader_basisu_pick_and_apply_compressed_format(
		ld,
		taisei_meta,
		file_info.source_format,
		bld.px_origin,
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

	if(pixmap_format_is_compressed(choosen_format)) {
		bld.px_decode_format = choosen_format;
		bld.is_uncompressed_fallback = false;
		p.format = compfmt_pixmap_to_basist(choosen_format);
	} else {
		bld.px_decode_format = px_fallback_format;
		bld.is_uncompressed_fallback = true;
		p.format = basis_fallback_format;
	}

	assume(file_info.total_images >= 1);
	basist_image_info img_infos[file_info.total_images];

	for(uint i = 0; i < ARRAY_SIZE(img_infos); ++i) {
		TRY(basist_transcoder_get_image_info, bld.tc, i, img_infos + i);
	}

	TRY_SILENT(texture_loader_basisu_check_consistency, ctx, bld.tc, &file_info, img_infos);
	TRY_SILENT(texture_loader_basisu_init_mipmaps, ctx, &bld, ld, &img_infos[0]);
	texture_loader_basisu_set_swizzle(ld, bld.px_decode_format, taisei_meta);

	bld.swizzle_supported = r_supports(RFEAT_TEXTURE_SWIZZLE);
	bld.transcoding_started = false;

	switch(ld->params.class) {
		case TEXTURE_CLASS_2D: {
			p.image_index = 0;
			for(uint mip = 0; mip < ld->params.mipmaps; ++mip) {
				p.level_index = mip + bld.mip_bias;
				TRY_SILENT(texture_loader_basisu_load_pixmap, ctx, &bld, ld, &p, ld->pixmaps + mip);
			}

			break;
		}

		case TEXTURE_CLASS_CUBEMAP:
			for(int face = 0; face < 6; ++face) {
				p.image_index = face;
				for(uint mip = 0; mip < ld->params.mipmaps; ++mip) {
					p.level_index = mip + bld.mip_bias;
					Pixmap *px = &ld->cubemaps[mip].faces[face];
					TRY_SILENT(texture_loader_basisu_load_pixmap, ctx, &bld, ld, &p, px);
				}
			}

			break;

		default: UNREACHABLE;
	}

	if(bld.is_uncompressed_fallback && !bld.swizzle_supported) {
		ld->params.swizzle = (SwizzleMask) { "rgba" };
	}

	TRY(basist_transcoder_stop_transcoding, bld.tc);

	// These are expected to be pre-applied if needed
	ld->preprocess.multiply_alpha = 0;
	ld->preprocess.apply_alphamap = 0;

	// Don't unset this one; it may be set in case we've fallen back to decompression
	// and the renderer has no sRGB sampling support for this texture type.
	// ld->preprocess.linearize = 0;

	texture_loader_basisu_cleanup(&bld);
	texture_loader_continue(ld);
}
