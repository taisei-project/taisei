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
#include "../api.h"
#include "resource/resource.h"
#include "resource/texture.h"
#include "../glcommon/vtable.h"

typedef struct TextureImpl {
	GLuint gl_handle;
	GLuint pbo;
	GLTextureTypeInfo *type_info;
} TextureImpl;

void gl33_texture_create(Texture *tex, const TextureParams *params);
void gl33_texture_invalidate(Texture *tex);
void gl33_texture_fill(Texture *tex, void *image_data);
void gl33_texture_fill_region(Texture *tex, uint x, uint y, uint w, uint h, void *image_data);
void gl33_texture_replace(Texture *tex, TextureType type, uint w, uint h, void *image_data);
void gl33_texture_destroy(Texture *tex);

GLTextureTypeInfo* gl33_texture_type_info(TextureType type);
