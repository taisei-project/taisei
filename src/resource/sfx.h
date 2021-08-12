/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"

extern ResourceHandler sfx_res_handler;

#define SFX_PATH_PREFIX "res/sfx/"

typedef struct SFX SFX;

DEFINE_OPTIONAL_RESOURCE_GETTER(SFX, res_sfx, RES_SFX)
