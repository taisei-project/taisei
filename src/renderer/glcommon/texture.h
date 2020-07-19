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

typedef struct GLTextureFormatTuple {
	GLuint gl_fmt;
	GLuint gl_type;
	PixmapFormat px_fmt;
} GLTextureFormatTuple;

typedef struct GLTextureTypeInfo {
	GLuint internal_fmt;
	GLTextureFormatTuple *external_formats;
	GLTextureFormatTuple primary_external_format;
} GLTextureTypeInfo;

GLTextureFormatTuple *glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt);
GLenum glcommon_texture_base_format(GLenum internal_fmt);
GLenum glcommon_compression_to_gl_format(PixmapCompression cfmt);
GLenum glcommon_compression_to_gl_format_srgb(PixmapCompression cfmt);
GLenum glcommon_uncompressed_format_to_srgb_format(GLenum format);

#endif // IGUARD_renderer_glcommon_texture_h
