/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_renderer_gl33_texture_h
#define IGUARD_renderer_gl33_texture_h

#include "taisei.h"

#include "gl33.h"
#include "../api.h"
#include "resource/resource.h"
#include "resource/texture.h"
#include "../glcommon/vtable.h"

typedef struct Texture {
	GLTextureTypeInfo *type_info;
	TextureUnit *binding_unit;
	GLuint gl_handle;
	GLuint pbo;
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
void gl33_texture_fill(Texture *tex, uint mipmap, const Pixmap *image);
void gl33_texture_fill_region(Texture *tex, uint mipmap, uint x, uint y, const Pixmap *image);
void gl33_texture_prepare(Texture *tex);
void gl33_texture_taint(Texture *tex);
void gl44_texture_clear(Texture *tex, const Color *clr);
void gl33_texture_clear(Texture *tex, const Color *clr);
void gl33_texture_destroy(Texture *tex);
bool gl33_texture_type_supported(TextureType type, TextureFlags flags);
PixmapFormat gl33_texture_optimal_pixmap_format_for_type(TextureType type, PixmapFormat src_format);

GLTextureTypeInfo *gl33_texture_type_info(TextureType type);
GLTexFormatCapabilities gl33_texture_format_caps(GLenum internal_fmt);

#endif // IGUARD_renderer_gl33_texture_h
