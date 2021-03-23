/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "basisu_cache.h"
#include "pixmap/serialize.h"
#include "rwops/rwops_zlib.h"

#include <basisu_transcoder_c_api.h>

// FIXME: The cache is not atomic.
// This may be a problem if multiple threads are loading identical .basis files,
// or if multiple Taisei instances are running for some reason.

enum {
	ENTRY_PATH_SIZE = 256,
};

static bool texture_loader_basisu_make_cache_path(
	const char *basisu_hash,
	const basist_transcode_level_params *tc_params,
	size_t bufsize,
	char buf[bufsize]
) {
	int len = snprintf(
		buf, bufsize,
		"cache/textures/basisu-%s/%u_%u_%s_%x",
		basisu_hash,
		tc_params->image_index,
		tc_params->level_index,
		basist_get_format_name(tc_params->format),
		tc_params->decode_flags
	);

	if(len >= bufsize) {
		log_error("Cache entry name is too long");
		return false;
	}

	return true;
}

bool texture_loader_basisu_load_cached(
	const char *basisu_hash,
	const basist_transcode_level_params *tc_params,
	const basist_image_level_desc *level_desc,
	PixmapFormat expected_px_format,
	PixmapOrigin expected_px_origin,
	uint32_t expected_size,
	Pixmap *out_pixmap
) {
	char path[ENTRY_PATH_SIZE];

	if(!texture_loader_basisu_make_cache_path(basisu_hash, tc_params, sizeof(path), path)) {
		return false;
	}

	if(!vfs_query(path).exists) {
		BASISU_DEBUG("%s not found", path);
		return false;
	}

	SDL_RWops *rw = vfs_open(path, VFS_MODE_READ);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	rw = SDL_RWWrapZlibReader(rw, 1 << 20, true);

	bool deserialize_ok = pixmap_deserialize(rw, out_pixmap);
	SDL_RWclose(rw);

	if(!deserialize_ok) {
		log_error("%s: Failed to deserialize cached pixmap", path);
		return false;
	}

	if(out_pixmap->format != expected_px_format) {
		const char *n = pixmap_format_name(out_pixmap->format);

		if(n) {
			log_error("%s: Bad cache entry: Expected format %s, got %s",
				path,
				pixmap_format_name(expected_px_format),
				n
			);
		} else {
			log_error("%s: Bad cache entry: Expected format %s, got 0x%04x",
				path,
				pixmap_format_name(expected_px_format),
				out_pixmap->format
			);
		}

		goto bad_entry;
	}

	if(out_pixmap->origin != expected_px_origin) {
		const char *origin_name[] = {
			[PIXMAP_ORIGIN_BOTTOMLEFT] = "BOTTOMLEFT",
			[PIXMAP_ORIGIN_TOPLEFT] = "TOPLEFT",
		};

		log_error("%s: Bad cache entry: Expected origin %s, got %s",
			path,
			origin_name[expected_px_origin],
			origin_name[out_pixmap->origin]
		);

		goto bad_entry;
	}

	if(
		out_pixmap->width != level_desc->orig_width ||
		out_pixmap->height != level_desc->orig_height
	) {
		log_error("%s: Bad cache entry: Expected dimensions %ux%u, got %ux%u",
			path,
			level_desc->orig_width,
			level_desc->orig_height,
			out_pixmap->width,
			out_pixmap->height
		);

		goto bad_entry;
	}

	if(out_pixmap->data_size != expected_size) {
		log_error("%s: Bad cache entry: Expected data size %u, got %u",
			path,
			expected_size,
			out_pixmap->data_size
		);

		goto bad_entry;
	}

	BASISU_DEBUG("Loaded cache entry from %s", path);
	return true;

bad_entry:
	free(out_pixmap->data.untyped);
	out_pixmap->data.untyped = NULL;
	return false;
}

bool texture_loader_basisu_cache(
	const char *basisu_hash,
	const basist_transcode_level_params *tc_params,
	const basist_image_level_desc *level_desc,
	const Pixmap *pixmap
) {
	char path[ENTRY_PATH_SIZE];

	if(!texture_loader_basisu_make_cache_path(basisu_hash, tc_params, sizeof(path), path)) {
		return false;
	}

	if(!vfs_mkparents(path)) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	SDL_RWops *rw = vfs_open(path, VFS_MODE_WRITE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	rw = SDL_RWWrapZlibWriter(rw, RW_DEFLATE_LEVEL_DEFAULT, 1 << 20, true);

	bool serialize_ok = pixmap_serialize(rw, pixmap);
	SDL_RWclose(rw);

	if(!serialize_ok) {
		log_error("%s: Failed to serialize pixmap", path);
		return false;
	}

	BASISU_DEBUG("Cached pixmap at %s", path);

	return true;
}
