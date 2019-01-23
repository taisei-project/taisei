/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "texture.h"
#include "util.h"
#include "../api.h"
#include "vtable.h"

static GLTextureFormatTuple* glcommon_find_best_pixformat_internal(GLTextureFormatTuple *formats, PixmapFormat pxfmt) {
	GLTextureFormatTuple *best = formats;

	if(best->px_fmt == pxfmt) {
		return best;
	}

	size_t ideal_channels = PIXMAP_FORMAT_LAYOUT(pxfmt);
	size_t ideal_depth = PIXMAP_FORMAT_DEPTH(pxfmt);
	bool ideal_is_float = PIXMAP_FORMAT_IS_FLOAT(pxfmt);
	bool best_is_float;

	for(GLTextureFormatTuple *fmt = formats + 1; fmt->px_fmt; ++fmt) {
		if(pxfmt == fmt->px_fmt) {
			return fmt;
		}

		size_t best_channels = PIXMAP_FORMAT_LAYOUT(best->px_fmt);
		size_t best_depth = PIXMAP_FORMAT_DEPTH(best->px_fmt);
		best_is_float = PIXMAP_FORMAT_IS_FLOAT(best->px_fmt);

		size_t this_channels = PIXMAP_FORMAT_LAYOUT(fmt->px_fmt);
		size_t this_depth = PIXMAP_FORMAT_DEPTH(fmt->px_fmt);
		bool this_is_float = PIXMAP_FORMAT_IS_FLOAT(fmt->px_fmt);

		if(best_channels < ideal_channels) {
			if(
				this_channels > best_channels || (
					this_channels == best_channels && (
						(this_is_float && ideal_is_float && !best_is_float) || this_depth >= ideal_depth
					)
				)
			) {
				best = fmt;
			}
		} else if(ideal_is_float && !best_is_float) {
			if(this_channels >= ideal_channels && (this_is_float || this_depth >= best_depth)) {
				best = fmt;
			}
		} else if(best_depth < ideal_depth) {
			if(this_channels >= ideal_channels && this_depth >= best_depth) {
				best = fmt;
			}
		} else if(
			this_channels >= ideal_channels &&
			this_depth >= ideal_depth &&
			this_is_float == ideal_is_float &&
			(this_channels < best_channels || this_depth < best_depth)
		) {
			best = fmt;
		}
	}

	log_debug("(%u, %u, %s) --> (%u, %u, %s)",
		PIXMAP_FORMAT_LAYOUT(pxfmt),
		PIXMAP_FORMAT_DEPTH(pxfmt),
		ideal_is_float ? "float" : "uint",
		PIXMAP_FORMAT_LAYOUT(best->px_fmt),
		PIXMAP_FORMAT_DEPTH(best->px_fmt),
		best_is_float ? "float" : "uint"
	);

	return best;
}

GLTextureFormatTuple* glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt) {
	GLTextureFormatTuple *formats = GLVT.texture_type_info(textype)->external_formats;
	return glcommon_find_best_pixformat_internal(formats, pxfmt);
}
