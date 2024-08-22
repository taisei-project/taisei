/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "texture.h"
#include "sdlgpu.h"

#include "../api.h"

#include "util.h"

static SDL_GpuTextureFormat sdlgpu_texfmt_ts2sdl(TextureType t) {
	static const SDL_GpuCompareOp mapping[] = {
		[TEX_TYPE_RGBA_8] = SDL_GPU_TEXTUREFORMAT_R8G8B8A8,
		[TEX_TYPE_RGB_8] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_RG_8] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_R_8] = SDL_GPU_TEXTUREFORMAT_R8,
		[TEX_TYPE_RGBA_16] = SDL_GPU_TEXTUREFORMAT_R16G16B16A16,
		[TEX_TYPE_RGB_16] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_RG_16] = SDL_GPU_TEXTUREFORMAT_R16G16,
		[TEX_TYPE_R_16] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_RGBA_16_FLOAT] = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_SFLOAT,
		[TEX_TYPE_RGB_16_FLOAT] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_RG_16_FLOAT] = SDL_GPU_TEXTUREFORMAT_R16G16_SFLOAT,
		[TEX_TYPE_R_16_FLOAT] = SDL_GPU_TEXTUREFORMAT_R16_SFLOAT,
		[TEX_TYPE_RGBA_32_FLOAT] = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_SFLOAT,
		[TEX_TYPE_RGB_32_FLOAT] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_RG_32_FLOAT] = SDL_GPU_TEXTUREFORMAT_R32G32_SFLOAT,
		[TEX_TYPE_R_32_FLOAT] = SDL_GPU_TEXTUREFORMAT_R32_SFLOAT,
		[TEX_TYPE_DEPTH_8] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_DEPTH_16] = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		[TEX_TYPE_DEPTH_24] = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
		[TEX_TYPE_DEPTH_32] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_DEPTH_16_FLOAT] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_DEPTH_32_FLOAT] = SDL_GPU_TEXTUREFORMAT_D32_SFLOAT,
		[TEX_TYPE_COMPRESSED_ETC1_RGB] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ETC2_RGBA] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_BC1_RGB] = SDL_GPU_TEXTUREFORMAT_BC1,
		[TEX_TYPE_COMPRESSED_BC3_RGBA] = SDL_GPU_TEXTUREFORMAT_BC3,
		[TEX_TYPE_COMPRESSED_BC4_R] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_BC5_RG] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_BC7_RGBA] = SDL_GPU_TEXTUREFORMAT_BC7,
		[TEX_TYPE_COMPRESSED_PVRTC1_4_RGB] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_PVRTC1_4_RGBA] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ASTC_4x4_RGBA] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ATC_RGB] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ATC_RGBA] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_PVRTC2_4_RGB] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_PVRTC2_4_RGBA] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ETC2_EAC_R11] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_ETC2_EAC_RG11] = SDL_GPU_TEXTUREFORMAT_INVALID,
		[TEX_TYPE_COMPRESSED_FXT1_RGB] = SDL_GPU_TEXTUREFORMAT_INVALID,
	};

	uint idx = t;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

static SDL_GpuTextureFormat sdlgpu_texfmt_to_srgb(SDL_GpuTextureFormat fmt) {
	switch(fmt) {
		case SDL_GPU_TEXTUREFORMAT_R8G8B8A8: return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SRGB;
		case SDL_GPU_TEXTUREFORMAT_B8G8R8A8: return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_SRGB;
		case SDL_GPU_TEXTUREFORMAT_BC3:      return SDL_GPU_TEXTUREFORMAT_BC3_SRGB;
		case SDL_GPU_TEXTUREFORMAT_BC7:      return SDL_GPU_TEXTUREFORMAT_BC7_SRGB;
		default: return SDL_GPU_TEXTUREFORMAT_INVALID;
	}
}

static TextureType sdlgpu_remap_texture_type(TextureType tt) {
	switch(tt) {
		case TEX_TYPE_RGB_8:				return TEX_TYPE_RGBA_8;
		case TEX_TYPE_RG_8:					return TEX_TYPE_RGBA_8;
		case TEX_TYPE_RGB_16:				return TEX_TYPE_RGBA_16;
		case TEX_TYPE_R_16:					return TEX_TYPE_RG_16;
		case TEX_TYPE_RGB_16_FLOAT:			return TEX_TYPE_RGB_16_FLOAT;
		case TEX_TYPE_RGB_32_FLOAT:			return TEX_TYPE_RGBA_32_FLOAT;
		case TEX_TYPE_DEPTH_8:				return TEX_TYPE_DEPTH_16;
		case TEX_TYPE_DEPTH_32:				return TEX_TYPE_DEPTH_24;
		case TEX_TYPE_DEPTH_16_FLOAT:		return TEX_TYPE_DEPTH_32_FLOAT;
		default:							return tt;
	}
}

static PixmapFormat sdlgpu_texfmt_to_pixfmt(SDL_GpuTextureFormat fmt) {
	switch(fmt) {
		case SDL_GPU_TEXTUREFORMAT_R8G8B8A8:
			return PIXMAP_FORMAT_RGBA8;
		case SDL_GPU_TEXTUREFORMAT_B8G8R8A8:
			return PIXMAP_FORMAT_RGBA8;  // FIXME can't represent swizzled formats yet!
		case SDL_GPU_TEXTUREFORMAT_R16G16:
			return PIXMAP_FORMAT_RG16;
		case SDL_GPU_TEXTUREFORMAT_R16G16B16A16:
			return PIXMAP_FORMAT_RGBA16;
		case SDL_GPU_TEXTUREFORMAT_R8:
			return PIXMAP_FORMAT_R8;
		case SDL_GPU_TEXTUREFORMAT_A8:
			return PIXMAP_FORMAT_R8;
		case SDL_GPU_TEXTUREFORMAT_BC1:
			return PIXMAP_FORMAT_BC1_RGB;
		case SDL_GPU_TEXTUREFORMAT_BC3:
			return PIXMAP_FORMAT_BC3_RGBA;
		case SDL_GPU_TEXTUREFORMAT_BC7:
			return PIXMAP_FORMAT_BC7_RGBA;
		case SDL_GPU_TEXTUREFORMAT_R8G8_SNORM:
			return PIXMAP_FORMAT_RG8;
		case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM:
			return PIXMAP_FORMAT_RGBA8;
		case SDL_GPU_TEXTUREFORMAT_R16_SFLOAT:
			return PIXMAP_FORMAT_R16F;
		case SDL_GPU_TEXTUREFORMAT_R16G16_SFLOAT:
			return PIXMAP_FORMAT_RG16F;
		case SDL_GPU_TEXTUREFORMAT_R16G16B16A16_SFLOAT:
			return PIXMAP_FORMAT_RGBA16F;
		case SDL_GPU_TEXTUREFORMAT_R32_SFLOAT:
			return PIXMAP_FORMAT_R32F;
		case SDL_GPU_TEXTUREFORMAT_R32G32_SFLOAT:
			return PIXMAP_FORMAT_RG32F;
		case SDL_GPU_TEXTUREFORMAT_R32G32B32A32_SFLOAT:
			return PIXMAP_FORMAT_RGB32F;
		case SDL_GPU_TEXTUREFORMAT_R8_UINT:
			return PIXMAP_FORMAT_R8;
		case SDL_GPU_TEXTUREFORMAT_R8G8_UINT:
			return PIXMAP_FORMAT_RG8;
		case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT:
			return PIXMAP_FORMAT_RGBA8;
		case SDL_GPU_TEXTUREFORMAT_R16_UINT:
			return PIXMAP_FORMAT_R16;
		case SDL_GPU_TEXTUREFORMAT_R16G16_UINT:
			return PIXMAP_FORMAT_RG16;
		case SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UINT:
			return PIXMAP_FORMAT_RGBA16;
		case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SRGB:
			return PIXMAP_FORMAT_RGBA8;
		case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_SRGB:
			return PIXMAP_FORMAT_RGBA8;  // FIXME can't represent swizzled formats yet!
		case SDL_GPU_TEXTUREFORMAT_BC3_SRGB:
			return PIXMAP_FORMAT_BC3_RGBA;
		case SDL_GPU_TEXTUREFORMAT_BC7_SRGB:
			return PIXMAP_FORMAT_BC7_RGBA;
		case SDL_GPU_TEXTUREFORMAT_D16_UNORM:
			return PIXMAP_FORMAT_R16;
		case SDL_GPU_TEXTUREFORMAT_D32_SFLOAT:
			return PIXMAP_FORMAT_R32F;

		case SDL_GPU_TEXTUREFORMAT_INVALID:
		case SDL_GPU_TEXTUREFORMAT_B5G6R5:
		case SDL_GPU_TEXTUREFORMAT_B5G5R5A1:
		case SDL_GPU_TEXTUREFORMAT_B4G4R4A4:
		case SDL_GPU_TEXTUREFORMAT_R10G10B10A2:
		case SDL_GPU_TEXTUREFORMAT_BC2:
		case SDL_GPU_TEXTUREFORMAT_D24_UNORM:
		case SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT:
		case SDL_GPU_TEXTUREFORMAT_D32_SFLOAT_S8_UINT:
			return 0;
	}

	UNREACHABLE;
}

static bool sdlgpu_texture_check_swizzle_component(char val, char defval) {
	return !val || val == defval;
}

static bool sdlgpu_texture_check_swizzle(const SwizzleMask *swizzle) {
	return
		sdlgpu_texture_check_swizzle_component(swizzle->r, 'r') &&
		sdlgpu_texture_check_swizzle_component(swizzle->g, 'g') &&
		sdlgpu_texture_check_swizzle_component(swizzle->b, 'b') &&
		sdlgpu_texture_check_swizzle_component(swizzle->a, 'a');
}

static SDL_GpuFilter sdlgpu_filter_ts2sdl(TextureFilterMode fm) {
	switch(fm) {
		case TEX_FILTER_LINEAR:
		case TEX_FILTER_LINEAR_MIPMAP_NEAREST:
		case TEX_FILTER_LINEAR_MIPMAP_LINEAR:
			return SDL_GPU_FILTER_LINEAR;
		case TEX_FILTER_NEAREST:
		case TEX_FILTER_NEAREST_MIPMAP_NEAREST:
		case TEX_FILTER_NEAREST_MIPMAP_LINEAR:
			return SDL_GPU_FILTER_NEAREST;
	}

	UNREACHABLE;
}

static SDL_GpuSamplerMipmapMode sdlgpu_mipmode_ts2sdl(TextureFilterMode fm) {
	switch(fm) {
		case TEX_FILTER_LINEAR_MIPMAP_LINEAR:
		case TEX_FILTER_NEAREST_MIPMAP_LINEAR:
			return SDL_GPU_FILTER_LINEAR;
		case TEX_FILTER_LINEAR_MIPMAP_NEAREST:
		case TEX_FILTER_NEAREST_MIPMAP_NEAREST:
		case TEX_FILTER_LINEAR:
		case TEX_FILTER_NEAREST:
			return SDL_GPU_FILTER_NEAREST;
	}

	UNREACHABLE;
}

static SDL_GpuTextureType sdlgpu_type_ts2sdl(TextureClass cls) {
	switch(cls) {
		case TEXTURE_CLASS_2D:
			return SDL_GPU_TEXTURETYPE_2D;
		case TEXTURE_CLASS_CUBEMAP:
			return SDL_GPU_TEXTURETYPE_CUBE;
	}

	UNREACHABLE;
}

static SDL_GpuSamplerMipmapMode sdlgpu_addrmode_ts2sdl(TextureWrapMode wm) {
	switch(wm) {
		case TEX_WRAP_REPEAT:	return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
		case TEX_WRAP_MIRROR:	return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
		case TEX_WRAP_CLAMP:	return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	}

	UNREACHABLE;
}

void sdlgpu_texture_update_sampler(Texture *tex) {
	if(tex->sampler && !tex->sampler_is_outdated) {
		return;
	}

	if(tex->sampler) {
		SDL_GpuReleaseSampler(sdlgpu.device, tex->sampler);
	}

	auto p = &tex->params;

	tex->sampler = SDL_GpuCreateSampler(sdlgpu.device, &(SDL_GpuSamplerCreateInfo) {
		.minFilter = sdlgpu_filter_ts2sdl(p->filter.min),
		.magFilter = sdlgpu_filter_ts2sdl(p->filter.mag),
		.mipmapMode = sdlgpu_mipmode_ts2sdl(p->filter.min),
		.addressModeU = sdlgpu_addrmode_ts2sdl(p->wrap.s),
		.addressModeV = sdlgpu_addrmode_ts2sdl(p->wrap.t),
		.addressModeW = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.mipLodBias = 0,
		.anisotropyEnable = p->anisotropy > 1,
		.maxAnisotropy = p->anisotropy,
		.compareEnable = 0,
		.compareOp = 0,
		.minLod = 0,
		.maxLod = p->mipmaps,
	});

	tex->sampler_is_outdated = false;
}

static bool is_depth_format(SDL_GpuTextureFormat fmt) {
	switch(fmt) {
		case SDL_GPU_TEXTUREFORMAT_D16_UNORM:
		case SDL_GPU_TEXTUREFORMAT_D24_UNORM:
		case SDL_GPU_TEXTUREFORMAT_D32_SFLOAT:
		case SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT:
		case SDL_GPU_TEXTUREFORMAT_D32_SFLOAT_S8_UINT:
			return true;
		default: return false;
	}
}

Texture *sdlgpu_texture_create(const TextureParams *params) {
	/*
	 * FIXME: a lot of the param validation code is more-or-less copypasted from gl33,
	 * we should organize it into a common utility function.
	 */

	auto tex = ALLOC(Texture, {
		.params = *params,
	});

	tex->params.type = sdlgpu_remap_texture_type(tex->params.type);
	SDL_GpuTextureFormat tex_fmt = sdlgpu_texfmt_ts2sdl(tex->params.type);

	if(UNLIKELY(tex->params.type == TEX_TYPE_INVALID)) {
		log_error("Requested unsupported texture type %s", r_texture_type_name(tex->params.type));
		goto fail;
	}

	assert(tex_fmt != SDL_GPU_TEXTUREFORMAT_INVALID);

	if(tex->params.flags & TEX_FLAG_SRGB) {
		SDL_GpuTextureFormat tex_fmt_srgb = sdlgpu_texfmt_to_srgb(tex_fmt);

		if(tex_fmt_srgb == SDL_GPU_TEXTUREFORMAT_INVALID) {
			log_error("No sRGB support for texture type %s (internal format %u)",
				r_texture_type_name(tex->params.type), tex_fmt);
			goto fail;
		}

		tex_fmt = tex_fmt_srgb;
	}

	if(!sdlgpu_texture_check_swizzle(&tex->params.swizzle)) {
		log_error("Swizzle masks are not supported");
		goto fail;
	}

	assert(tex->params.width > 0);
	assert(tex->params.height > 0);

	if(tex->params.class == TEXTURE_CLASS_CUBEMAP) {
		assert(tex->params.width == tex->params.height);
		assert(tex->params.layers == 6);
	} else if(tex->params.class == TEXTURE_CLASS_2D) {
		if(tex->params.layers == 0) {
			tex->params.layers = 1;
		}

		assert(tex->params.layers == 1);
	} else {
		UNREACHABLE;
	}

	uint max_mipmaps = r_texture_util_max_num_miplevels(tex->params.width, tex->params.height);
	assert(max_mipmaps > 0);

	if(tex->params.mipmaps == 0) {
		if(tex->params.mipmap_mode == TEX_MIPMAP_AUTO) {
			tex->params.mipmaps = TEX_MIPMAPS_MAX;
		} else {
			tex->params.mipmaps = 1;
		}
	}

	if(tex->params.mipmaps == TEX_MIPMAPS_MAX || tex->params.mipmaps > max_mipmaps) {
		tex->params.mipmaps = max_mipmaps;
	}

	if(tex->params.anisotropy == 0) {
		tex->params.anisotropy = TEX_ANISOTROPY_DEFAULT;
	}

	SDL_GpuTextureUsageFlagBits usage = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT;

	if(is_depth_format(tex_fmt)) {
		usage |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET_BIT;
	} else {
		// FIXME: we don't know whether we're going to need to render to this texture or not
		usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET_BIT;
	}

	tex->gpu_texture = SDL_GpuCreateTexture(sdlgpu.device, &(SDL_GpuTextureCreateInfo) {
		.type = sdlgpu_type_ts2sdl(tex->params.class),
		.format = tex_fmt,
		.width = tex->params.width,
		.height = tex->params.height,
		.layerCountOrDepth = tex->params.layers,
		.levelCount = tex->params.mipmaps,
		.usageFlags = usage,
	});

	if(UNLIKELY(!tex->gpu_texture)) {
		log_sdl_error(LOG_ERROR, "SDL_GpuCreateTexture");
		goto fail;
	}

	return tex;

fail:
	mem_free(tex);
	return tex;
}

void sdlgpu_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height) {
	// TODO: copypasted from gl33; move to common code?

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
			*width = max(1, tex->params.width / (1u << mipmap));
		}

		if(height != NULL) {
			*height = max(1, tex->params.height / (1u << mipmap));
		}
	}
}

void sdlgpu_texture_get_params(Texture *tex, TextureParams *params) {
	*params = tex->params;
}

const char *sdlgpu_texture_get_debug_label(Texture *tex) {
	return tex->debug_label;
}

void sdlgpu_texture_set_debug_label(Texture *tex, const char *label) {
	strlcpy(tex->debug_label, label, sizeof(tex->debug_label));
	SDL_GpuSetTextureName(sdlgpu.device, tex->gpu_texture, label);
}

void sdlgpu_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag) {
	UNREACHABLE;
}

void sdlgpu_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt) {
	UNREACHABLE;
}

void sdlgpu_texture_invalidate(Texture *tex) {
	UNREACHABLE;
}

void sdlgpu_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image) {
	// TODO persistent transfer buffer for streamed textures
	SDL_GpuTransferBuffer *tbuf = SDL_GpuCreateTransferBuffer(sdlgpu.device, &(SDL_GpuTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.sizeInBytes = image->data_size,
	});

	uint8_t *mapped = SDL_GpuMapTransferBuffer(sdlgpu.device, tbuf, SDL_FALSE);
	memcpy(mapped, image->data.untyped, image->data_size);
	SDL_GpuUnmapTransferBuffer(sdlgpu.device, tbuf);

	SDL_GpuCopyPass *copy_pass = SDL_GpuBeginCopyPass(sdlgpu.frame.cbuf);

	SDL_GpuUploadToTexture(copy_pass,
		&(SDL_GpuTextureTransferInfo) {
			.transferBuffer = tbuf,
		},
		&(SDL_GpuTextureRegion) {
			.texture = tex->gpu_texture,
			.w = image->width,
			.h = image->height,
			.d = 1,
		},
	false);

	SDL_GpuEndCopyPass(copy_pass);
	SDL_GpuReleaseTransferBuffer(sdlgpu.device, tbuf);
}

void sdlgpu_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image) {
	UNREACHABLE;
}

void sdlgpu_texture_prepare(Texture *tex) {
	UNREACHABLE;
}

void sdlgpu_texture_taint(Texture *tex) {
	UNREACHABLE;
}

void sdlgpu_texture_clear(Texture *tex, const Color *clr) {
	UNREACHABLE;
}

void sdlgpu_texture_destroy(Texture *tex) {
	SDL_GpuReleaseTexture(sdlgpu.device, tex->gpu_texture);
	SDL_GpuReleaseSampler(sdlgpu.device, tex->sampler);
	mem_free(tex);
}

bool sdlgpu_texture_type_query(
	TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin,
	TextureTypeQueryResult *result
) {
	type = sdlgpu_remap_texture_type(type);
	SDL_GpuTextureFormat format = sdlgpu_texfmt_ts2sdl(type);

	if(flags & TEX_FLAG_SRGB) {
		format = sdlgpu_texfmt_to_srgb(format);
	}

	if(format == SDL_GPU_TEXTUREFORMAT_INVALID) {
		return false;
	}

	PixmapFormat pixfmt = sdlgpu_texfmt_to_pixfmt(format);

	if(pixfmt == 0) {
		// FIXME: supported but can't upload or download, what to do here?
		return false;
	}

	if(!result) {
		return true;
	}

	result->optimal_pixmap_format = pixfmt;
	result->optimal_pixmap_origin = PIXMAP_ORIGIN_TOPLEFT;

	result->supplied_pixmap_origin_supported = (pxorigin == result->supplied_pixmap_origin_supported);
	result->supplied_pixmap_format_supported = (pxfmt == result->supplied_pixmap_format_supported);

	return true;
}

bool sdlgpu_texture_sampler_compatible(Texture *tex, UniformType sampler_type) {
	switch(tex->params.class) {
		case TEXTURE_CLASS_CUBEMAP:	return sampler_type == UNIFORM_SAMPLER_CUBE;
		case TEXTURE_CLASS_2D:		return sampler_type == UNIFORM_SAMPLER_2D;
	}

	UNREACHABLE;
}

bool sdlgpu_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst) {
	UNREACHABLE;
}

bool sdlgpu_texture_transfer(Texture *dst, Texture *src) {
	UNREACHABLE;
}
