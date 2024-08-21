/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

struct Texture {
	SDL_GpuTexture *gpu_texture;
	SDL_GpuSampler *sampler;
	TextureParams params;
	bool sampler_is_outdated;
	char debug_label[R_DEBUG_LABEL_SIZE];
};


Texture *sdlgpu_texture_create(const TextureParams *params);
void sdlgpu_texture_get_size(Texture *tex, uint mipmap, uint *width, uint *height);
void sdlgpu_texture_get_params(Texture *tex, TextureParams *params);
const char *sdlgpu_texture_get_debug_label(Texture *tex);
void sdlgpu_texture_set_debug_label(Texture *tex, const char *label);
void sdlgpu_texture_set_filter(Texture *tex, TextureFilterMode fmin, TextureFilterMode fmag);
void sdlgpu_texture_set_wrap(Texture *tex, TextureWrapMode ws, TextureWrapMode wt);
void sdlgpu_texture_invalidate(Texture *tex);
void sdlgpu_texture_fill(Texture *tex, uint mipmap, uint layer, const Pixmap *image);
void sdlgpu_texture_fill_region(Texture *tex, uint mipmap, uint layer, uint x, uint y, const Pixmap *image);
void sdlgpu_texture_update_sampler(Texture *tex);
void sdlgpu_texture_prepare(Texture *tex);
void sdlgpu_texture_taint(Texture *tex);
void gl44_texture_clear(Texture *tex, const Color *clr);
void sdlgpu_texture_clear(Texture *tex, const Color *clr);
void sdlgpu_texture_destroy(Texture *tex);
bool sdlgpu_texture_type_query(TextureType type, TextureFlags flags, PixmapFormat pxfmt, PixmapOrigin pxorigin, TextureTypeQueryResult *result);
bool sdlgpu_texture_sampler_compatible(Texture *tex, UniformType sampler_type) attr_nonnull(1);
bool sdlgpu_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst);
bool sdlgpu_texture_transfer(Texture *dst, Texture *src);

