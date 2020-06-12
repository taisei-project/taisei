/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_bgm_h
#define IGUARD_resource_bgm_h

#include "taisei.h"

#include "resource.h"

extern ResourceHandler bgm_res_handler;

#define BGM_PATH_PREFIX "res/bgm/"

typedef struct BGM BGM;

DEFINE_OPTIONAL_RESOURCE_GETTER(BGM, res_bgm, RES_BGM)

const char *bgm_get_title(BGM *bgm) attr_nonnull(1);
const char *bgm_get_artist(BGM *bgm) attr_nonnull(1);
const char *bgm_get_comment(BGM *bgm) attr_nonnull(1);
double bgm_get_duration(BGM *bgm) attr_nonnull(1);
double bgm_get_loop_start(BGM *bgm) attr_nonnull(1);

#endif // IGUARD_resource_bgm_h
