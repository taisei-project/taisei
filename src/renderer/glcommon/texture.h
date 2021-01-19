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
#include "pixmap/pixmap.h"
#include "../api.h"

typedef enum GLTextureFormatFlags {
	GLTEX_COLOR_RENDERABLE = (1 << 0),
	GLTEX_DEPTH_RENDERABLE = (1 << 1),
	GLTEX_FILTERABLE       = (1 << 2),
	GLTEX_BLENDABLE        = (1 << 3),
	GLTEX_SRGB             = (1 << 4),
	GLTEX_COMPRESSED       = (1 << 5),
	GLTEX_FLOAT            = (1 << 6),
// 	GLTEX_MIPMAP           = (1 << 7),  TODO
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

typedef struct GLTextureFormatMatchConfig {
	TextureType intended_type;
	struct {
		GLTextureFormatFlags required;
		GLTextureFormatFlags forbidden;
		GLTextureFormatFlags desirable;
		GLTextureFormatFlags undesirable;
	} flags;
} GLTextureFormatMatchConfig;

void glcommon_init_texture_formats(void);
void glcommon_free_texture_formats(void);

GLTextureFormatInfo *glcommon_match_format(const GLTextureFormatMatchConfig *cfg);

#endif // IGUARD_renderer_glcommon_texture_h
