/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "basisu.h"

#include <basisu_transcoder_c_api.h>

bool texture_loader_basisu_load_cached(
	const char *basisu_hash,
	const basist_transcode_level_params *tc_params,
	const basist_image_level_desc *level_desc,
	PixmapFormat expected_px_format,
	PixmapOrigin expected_px_origin,
	uint32_t expected_size,
	Pixmap *out_pixmap
) attr_nonnull_all attr_nodiscard;

bool texture_loader_basisu_cache(
	const char *basisu_hash,
	const basist_transcode_level_params *tc_params,
	const basist_image_level_desc *level_desc,
	const Pixmap *pixmap
) attr_nonnull_all;
