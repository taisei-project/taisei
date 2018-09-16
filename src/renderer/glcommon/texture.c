/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
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

	for(GLTextureFormatTuple *fmt = formats + 1; fmt->px_fmt; ++fmt) {
		if(pxfmt == fmt->px_fmt) {
			return fmt;
		}

		size_t best_channels = PIXMAP_FORMAT_LAYOUT(best->px_fmt);
		size_t best_depth = PIXMAP_FORMAT_DEPTH(best->px_fmt);

		size_t this_channels = PIXMAP_FORMAT_LAYOUT(fmt->px_fmt);
		size_t this_depth = PIXMAP_FORMAT_DEPTH(fmt->px_fmt);

		if(best_channels < ideal_channels) {
			if(this_channels > best_channels || (this_channels == best_channels && this_depth >= ideal_depth)) {
				best = fmt;
			}
		} else if(best_depth < ideal_depth) {
			if(this_channels >= ideal_channels && this_depth >= best_depth) {
				best = fmt;
			}
		} else if(
			this_channels >= ideal_channels && this_depth >= ideal_depth &&
			(this_channels < best_channels || this_depth < best_depth)
		) {
			best = fmt;
		}
	}

	log_debug("(%u, %u) --> (%u, %u)",
		PIXMAP_FORMAT_LAYOUT(pxfmt),
		PIXMAP_FORMAT_DEPTH(pxfmt),
		PIXMAP_FORMAT_LAYOUT(best->px_fmt),
		PIXMAP_FORMAT_DEPTH(best->px_fmt)
	);

	return best;
}

GLTextureFormatTuple* glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt) {
	GLTextureFormatTuple *formats = GLVT.texture_type_info(textype)->external_formats;
	return glcommon_find_best_pixformat_internal(formats, pxfmt);
}
