/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_renderer_glcommon_texture_h
#define IGUARD_renderer_glcommon_texture_h

#include "taisei.h"

#include "opengl.h"
#include "util/pixmap.h"
#include "../api.h"

typedef enum GLTextureFormatFlags {
	GLTEX_COLOR_RENDERABLE = (1 << 0),
	GLTEX_DEPTH_RENDERABLE = (1 << 1),
	GLTEX_FILTERABLE       = (1 << 2),
	GLTEX_BLENDABLE        = (1 << 3),
	GLTEX_SRGB             = (1 << 4),
	GLTEX_COMPRESSED       = (1 << 5),
} GLTextureFormatFlags;

typedef struct GLTextureTransferFormatInfo {
	GLenum gl_format;
	GLenum gl_type;
	PixmapFormat pixmap_format;
} GLTextureTransferFormatInfo;

typedef struct GLTextureFormatInfo {
	TextureType intended_type_mapping;
	GLenum base_format;
	GLenum internal_format;
	GLTextureTransferFormatInfo transfer_format;
	GLTextureFormatFlags flags;
	uchar bits_per_pixel;
} GLTextureFormatInfo;

void glcommon_init_texture_formats(void);
void glcommon_free_texture_formats(void);

GLTextureFormatInfo *glcommon_match_format(TextureType tex_type, GLTextureFormatFlags require_flags, GLTextureFormatFlags forbid_flags);

#endif // IGUARD_renderer_glcommon_texture_h
