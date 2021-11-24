/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "gl33.h"
#include "../api.h"
#include "resource/resource.h"
#include "resource/texture.h"
#include "../glcommon/vtable.h"

typedef struct Texture {
	GLTextureFormatInfo *fmt_info;
	TextureUnit *binding_unit;
	GLuint gl_handle;
	GLuint pbo;
	GLenum bind_target;
	TextureParams params;
	bool mipmaps_outdated;
	char debug_label[R_DEBUG_LABEL_SIZE];
} TextureImpl;

Texture *gl33_texture_create(const TextureParams *params);
void gl33_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height);
void gl33_texture_get_params(Texture *tex, TextureParams *params);
const char *gl33_texture_get_debug_label(Texture *tex);
void gl33_texture_set_debug_label(Texture *tex, const char *label);
void gl33_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag);
void gl33_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt);
void gl33_texture_invalidate(Texture *tex);
void gl33_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image);
void gl33_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image);
void gl33_texture_prepare(Texture *tex);
void gl33_texture_taint(Texture *tex);
void gl44_texture_clear(Texture *tex, const Color *clr);
void gl33_texture_clear(Texture *tex, const Color *clr);
void gl33_texture_destroy(Texture *tex);
bool gl33_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result);
bool gl33_texture_sampler_compatible(Texture *tex, UniformType sampler_type) attr_nonnull(1);
bool gl33_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst);
bool gl33_texture_transfer(Texture *dst, Texture *src);
