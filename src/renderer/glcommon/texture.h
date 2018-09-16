/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
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
	size_t internal_pixel_size;
	GLTextureFormatTuple *external_formats;
	GLTextureFormatTuple primary_external_format;
} GLTextureTypeInfo;

GLTextureFormatTuple* glcommon_find_best_pixformat(TextureType textype, PixmapFormat pxfmt);
