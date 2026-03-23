/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "sprite.h"

extern ResourceHandler atlas_res_handler;

typedef struct Atlas Atlas;

bool atlas_get_sprite(Atlas *atlas, const char *name, Sprite *sprite) attr_nonnull_all;

#define ATLAS_PATH_PREFIX "res/gfx/"
#define ATLAS_EXTENSION ".tsatlas"

DEFINE_RESOURCE_GETTER(Atlas, res_atlas, RES_ATLAS)
DEFINE_OPTIONAL_RESOURCE_GETTER(Atlas, res_atlas_optional, RES_ATLAS)
