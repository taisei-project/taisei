/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "atlas.h"
#include "texture.h"

#include "memory/scratch.h"
#include "pixmap/pixmap.h"
#include "renderer/api.h"
#include "rwops/rwops_util.h"

#define TSATLAS_MAGIC 0xa7baacaab2b6baadull
#define TSATLAS_VERSION 1

#define TSATLAS_GFLAGS_KNOWN    (0b0u)
#define TSATLAS_GFLAGS_RESERVED ~TSATLAS_GFLAGS_KNOWN

typedef enum TSAtlasSpriteFlag {
	TSATLAS_SFLAG_FILTER_SUBTRACT_GREEN = (1u << 0),
	TSATLAS_SFLAG_ROTATED               = (1u << 1),
} TSAtlasSpriteFlag;

#define TSATLAS_SFLAGS_KNOWN    (\
	TSATLAS_SFLAG_FILTER_SUBTRACT_GREEN | \
	TSATLAS_SFLAG_ROTATED | \
0)
#define TSATLAS_SFLAGS_RESERVED ~TSATLAS_SFLAGS_KNOWN

typedef struct __attribute__((packed)) TSAtlasHeader {
	uint64_t magic;
	uint16_t version;
	uint16_t width;
	uint16_t height;
	uint16_t num_sprites;
	uint32_t flags;
} TSAtlasHeader;

typedef struct __attribute__((packed)) TSAtlasSpriteEntry {
	// uint8_t name_len;
	// char name[name_len];

	uint32_t flags;

	struct {
		uint16_t offset_x;
		uint16_t offset_y;
		uint16_t width;
		uint16_t height;
	} image;

	struct {
		uint16_t offset_x;
		uint16_t offset_y;
		uint16_t width;
		uint16_t height;
	} tex_region;

	struct {
		float width;
		float height;
	} size;

	struct {
		float top;
		float bottom;
		float left;
		float right;
	} padding;
} TSAtlasSpriteEntry;

struct Atlas {
	uint16_t width;
	uint16_t height;
	uint16_t num_sprites;

	cmplxf uv_scale;

	union {
		Texture *texture;
		uint8_t *pixel_buffer;
	};

	ht_str2ptr_t sprite_map;
	alignas(alignof(float)) TSAtlasSpriteEntry sprites[];
};

static char *atlas_find(const char *name) {
	return strjoin(ATLAS_PATH_PREFIX, name, ATLAS_EXTENSION, NULL);
}

static bool atlas_check(const char *path) {
	return strendswith(path, ATLAS_EXTENSION);
}

static void atlas_load_stage1(ResourceLoadState *st);
static void atlas_load_stage2(ResourceLoadState *st);

static void atlas_load_stage1(ResourceLoadState *st) {
	auto io = vfs_open(st->path, VFS_MODE_READ);

	if(UNLIKELY(!io)) {
		log_error("vfs_open() failed: %s", vfs_get_error());
		res_load_failed(st);
		return;
	}

	TSAtlasHeader head = {};

	#define IO_ERROR(fmt, ...) ({ \
		if(SDL_GetIOStatus(io) == SDL_IO_STATUS_EOF) { \
			log_error("%s: Unexpected EOF reading " fmt, iostream_get_name(io), ##__VA_ARGS__); \
		} else { \
			log_error("%s: Error reading " fmt ": %s", iostream_get_name(io), ##__VA_ARGS__, SDL_GetError()); \
		} \
	})

	if(UNLIKELY(SDL_ReadIO(io, &head, sizeof(head)) != sizeof(head))) {
		IO_ERROR("header");
		SDL_CloseIO(io);
		res_load_failed(st);
		return;
	}

	head.magic = SDL_Swap64LE(head.magic);
	head.version = SDL_Swap16LE(head.version);
	head.width = SDL_Swap16LE(head.width);
	head.height = SDL_Swap16LE(head.height);
	head.num_sprites = SDL_Swap16LE(head.num_sprites);
	head.flags = SDL_Swap32LE(head.flags);

	if(UNLIKELY(head.magic != TSATLAS_MAGIC)) {
		log_error("%s: Incorrect magic number: %llx", iostream_get_name(io), (unsigned long long)head.magic);
		SDL_CloseIO(io);
		res_load_failed(st);
		return;
	}

	if(UNLIKELY(head.version != TSATLAS_VERSION)) {
		log_error("%s: Unsupported format version: %d", iostream_get_name(io), head.version);
		SDL_CloseIO(io);
		res_load_failed(st);
		return;
	}

	uint32_t badflags = head.flags & TSATLAS_GFLAGS_RESERVED;
	if(UNLIKELY(badflags)) {
		log_error("%s: Unknown global flag bits set: 0x%08x", iostream_get_name(io), badflags);
	}

	if(UNLIKELY(head.width > INT16_MAX || head.height > INT16_MAX)) {
		log_error("%s: Texture size %dx%d is too large", iostream_get_name(io), head.width, head.height);
		SDL_CloseIO(io);
		res_load_failed(st);
		return;
	}

	auto atlas = ALLOC_FLEX(Atlas, sizeof(TSAtlasSpriteEntry) * head.num_sprites);
	atlas->width = head.width;
	atlas->height = head.height;
	atlas->uv_scale = CMPLXF(1.0f / atlas->width, 1.0f / atlas->height);
	atlas->num_sprites = head.num_sprites;
	ht_create(&atlas->sprite_map);

	auto scratch = acquire_scratch_arena();
	auto snap = marena_snapshot(scratch);

	for(uint i = 0; i < atlas->num_sprites; ++i, marena_rollback(scratch, &snap)) {
		uint8_t name_len;

		if(UNLIKELY(!SDL_ReadU8(io, &name_len))) {
			IO_ERROR("sprite %d", i);
			goto fail;
		}

		char *name = marena_alloc(scratch, name_len + 1);

		if(UNLIKELY(SDL_ReadIO(io, name, name_len) != name_len)) {
			IO_ERROR("sprite %d", i);
			goto fail;
		}

		name[name_len] = 0;

		if(UNLIKELY(strlen(name) != name_len)) {
			log_error("%s: Sprite %d name contains null characters (likely corrupted)", iostream_get_name(io), i);
			goto fail;
		}

		auto sprite = atlas->sprites + i;

		if(UNLIKELY(!ht_try_set(&atlas->sprite_map, name, sprite, NULL, NULL))) {
			log_error("%s: Sprite %d has non-unique name %s", iostream_get_name(io), i, name);
			goto fail;
		}

		if(UNLIKELY(SDL_ReadIO(io, sprite, sizeof(*sprite)) != sizeof(*sprite))) {
			IO_ERROR("sprite %d (%s)", i, name);
			goto fail;
		}

		sprite->flags = SDL_Swap32LE(sprite->flags);

		badflags = sprite->flags & TSATLAS_SFLAGS_RESERVED;
		if(UNLIKELY(badflags)) {
			log_error("%s: Sprite %d (%s) has unknown flag bits set: 0x%08x", iostream_get_name(io), i, name, badflags);
			goto fail;
		}

		if(UNLIKELY(sprite->flags & TSATLAS_SFLAG_ROTATED)) {
			log_error("%s: Sprite %d (%s) is rotated, this is not yet supported", iostream_get_name(io), i, name);
			goto fail;
		}

		sprite->image.offset_x = SDL_Swap16LE(sprite->image.offset_x);
		sprite->image.offset_y = SDL_Swap16LE(sprite->image.offset_y);
		sprite->image.width    = SDL_Swap16LE(sprite->image.width);
		sprite->image.height   = SDL_Swap16LE(sprite->image.height);

		if(UNLIKELY(
			(uint32_t)(sprite->image.offset_x + sprite->image.width)  > atlas->width ||
			(uint32_t)(sprite->image.offset_y + sprite->image.height) > atlas->height)
		) {
			log_error("%s: Sprite %d (%s) has invalid offset or dimensions", iostream_get_name(io), i, name);
			goto fail;
		}

		sprite->tex_region.offset_x = SDL_Swap16LE(sprite->tex_region.offset_x);
		sprite->tex_region.offset_y = SDL_Swap16LE(sprite->tex_region.offset_y);
		sprite->tex_region.width    = SDL_Swap16LE(sprite->tex_region.width);
		sprite->tex_region.height   = SDL_Swap16LE(sprite->tex_region.height);

		sprite->size.width  = SDL_SwapFloatLE(sprite->size.width);
		sprite->size.height = SDL_SwapFloatLE(sprite->size.height);

		sprite->padding.top    = SDL_SwapFloatLE(sprite->padding.top);
		sprite->padding.bottom = SDL_SwapFloatLE(sprite->padding.bottom);
		sprite->padding.left   = SDL_SwapFloatLE(sprite->padding.left);
		sprite->padding.right  = SDL_SwapFloatLE(sprite->padding.right);

		log_debug("Sprite #%d: %s  %dx%d", i, name, sprite->tex_region.width, sprite->tex_region.height);
	}

	PixmapFormat fmt = PIXMAP_FORMAT_RGBA8;
	atlas->pixel_buffer = pixmap_alloc_buffer(fmt, atlas->width, atlas->height, NULL);
	const size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(fmt);

	for(uint i = 0; i < atlas->num_sprites; ++i) {
		auto sprite = atlas->sprites + i;

		size_t row = sprite->image.offset_y;
		size_t col = sprite->image.offset_x;
		size_t row_size = sprite->image.width * pixel_size;

		auto rowp = (PixelRGBA8*)atlas->pixel_buffer + col + atlas->width * row;

		for(uint r = 0; r < sprite->image.height; ++r) {
			assert((uint8_t*)rowp >= atlas->pixel_buffer);

			auto read_size = SDL_ReadIO(io, rowp, row_size);

			// log_debug("read_size = %zu", read_size);

			if(UNLIKELY(read_size != row_size)) {
				IO_ERROR("sprite %d pixel data, row %d", i, r);
				goto fail;
			}

			if(sprite->flags & TSATLAS_SFLAG_FILTER_SUBTRACT_GREEN) {
				auto begin = rowp;
				auto end = begin + sprite->image.width;

				for(auto pixel = begin; pixel < end; ++pixel) {
					uint8_t a = pixel->values[0];
					uint8_t g = pixel->values[1];
					uint8_t r_minus_g = pixel->values[2];
					uint8_t b_minus_g = pixel->values[3];
					uint8_t r = r_minus_g + g;
					uint8_t b = b_minus_g + g;
					*pixel = (PixelRGBA8) { r, g, b, a };
				}
			}

			rowp += atlas->width;
		}

	}

	SDL_CloseIO(io);
	release_scratch_arena(scratch);
	res_load_continue_on_main(st, atlas_load_stage2, atlas);
	return;

fail:
	SDL_CloseIO(io);
	release_scratch_arena(scratch);
	mem_free(atlas->pixel_buffer);
	atlas->pixel_buffer = NULL;

	if(atlas->sprite_map.elements) {
		ht_destroy(&atlas->sprite_map);
	}

	mem_free(atlas);
	res_load_failed(st);
	return;
}

static void atlas_load_stage2(ResourceLoadState *st) {
	Atlas *atlas = NOT_NULL(st->opaque);

	Pixmap px = {
		.data = atlas->pixel_buffer,
		.format = PIXMAP_FORMAT_RGBA8,
		.width = atlas->width,
		.height = atlas->height,
	};

	px.data_size = pixmap_data_size(px.format, px.width, px.height);

	StringBuffer buf = { acquire_scratch_arena() };
#if 0
	strbuf_printf(&buf, "/tmp/__taisei_%s.png", st->name);
	char *fname = strbuf_commit(&buf);
	PixmapPNGSaveOptions png_opts = PIXMAP_DEFAULT_PNG_SAVE_OPTIONS;
	png_opts.base.file_format = PIXMAP_FILEFORMAT_PNG;
	auto io = SDL_IOFromFile(fname, "wb");
	pixmap_save_stream(io, &px, &png_opts.base);
	SDL_CloseIO(io);
#endif

	auto texture = r_texture_create(&(TextureParams) {
		.class = TEXTURE_CLASS_2D,
		.filter.min = TEX_FILTER_LINEAR_MIPMAP_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.width = px.width,
		.height = px.height,
		.layers = 1,
		.mipmaps = TEX_MIPMAP_AUTO,
		.type = TEX_TYPE_RGBA_8,
		.wrap.s = TEX_WRAP_REPEAT,
		.wrap.t = TEX_WRAP_REPEAT,
		.anisotropy = 1,
	});

	strbuf_printf(&buf, "atlas %s", st->name);
	r_texture_set_debug_label(texture, strbuf_commit(&buf));
	release_scratch_arena(buf.arena);
	r_texture_fill(texture, 0, 0, &px);

	mem_free(atlas->pixel_buffer);
	atlas->texture = texture;

	res_load_finished(st, atlas);
}

static void atlas_unload(void *vatlas) {
	Atlas *atlas = vatlas;
	r_texture_destroy(atlas->texture);
	ht_destroy(&atlas->sprite_map);
	mem_free(atlas);
}

bool atlas_get_sprite(Atlas *atlas, const char *name, Sprite *sprite) {
	TSAtlasSpriteEntry *aspr = ht_get(&atlas->sprite_map, name, NULL);

	if(!aspr) {
		return false;
	}

	cmplxf s = atlas->uv_scale;

	*sprite = (Sprite) {
		.tex = atlas->texture,
		.tex_area = {
			.extent.as_cmplx =
				cwmulf(s, CMPLXF(aspr->tex_region.width, aspr->tex_region.height)),
			.offset.as_cmplx =
				cwmulf(s, CMPLXF(aspr->tex_region.offset_x, aspr->tex_region.offset_y)),
		},
		.padding = {
			.extent = {
				.w = aspr->padding.left + aspr->padding.right,
				.h = aspr->padding.top + aspr->padding.bottom,
			},
			.offset = {
				.x = 0.5f * (aspr->padding.left - aspr->padding.right),
				.y = 0.5f * (aspr->padding.top - aspr->padding.bottom),
			}
		},
		.extent = {
			.w = aspr->size.width,
			.h = aspr->size.height,
		},
	};

	sprite->extent.as_cmplx += sprite->padding.extent.as_cmplx;

	return true;
}

ResourceHandler atlas_res_handler = {
	.type = RES_ATLAS,
	.typename = "atlas",
	.subdir = ATLAS_PATH_PREFIX,

	.procs = {
		.find = atlas_find,
		.check = atlas_check,
		.load = atlas_load_stage1,
		.unload = atlas_unload,
	},
};
