/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource/resource.h"
#include "resource/sprite.h"

#define PORTRAIT_PREFIX "dialog/"
#define PORTRAIT_VARIANT_SUFFIX "_variant_"
#define PORTRAIT_FACE_SUFFIX "_face_"

#define PORTRAIT_STATIC_FACE_SPRITE_NAME(charname, face) \
	PORTRAIT_PREFIX #charname PORTRAIT_FACE_SUFFIX #face

#define PORTRAIT_STATIC_VARIANT_SPRITE_NAME(charname, face) \
	PORTRAIT_PREFIX #charname PORTRAIT_VARIANT_SUFFIX #face

int portrait_get_base_sprite_name(const char *charname, const char *variant, size_t bufsize, char buf[bufsize])
	attr_nonnull(1, 4);

Sprite *portrait_get_base_sprite(const char *charname, const char *variant)
	attr_nonnull(1) attr_returns_nonnull;

void portrait_preload_base_sprite(ResourceGroup *rg, const char *charname, const char *variant, ResourceFlags rflags)
	attr_nonnull(2);

int portrait_get_face_sprite_name(const char *charname, const char *face, size_t bufsize, char buf[bufsize])
	attr_nonnull(1, 2, 4);

void portrait_preload_face_sprite(ResourceGroup *rg, const char *charname, const char *variant, ResourceFlags rflags)
	attr_nonnull(2, 3);

Sprite *portrait_get_face_sprite(const char *charname, const char *face)
	attr_nonnull(1, 2) attr_returns_nonnull;

void portrait_render(Sprite *s_base, Sprite *s_face, Sprite *s_out)
	attr_nonnull_all;

void portrait_render_byname(const char *charname, const char *variant, const char *face, Sprite *s_out)
	attr_nonnull(1, 3, 4);
