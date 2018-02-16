/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "bgm.h"

ResourceHandler bgm_res_handler = {
    .type = RES_ANIM,
    .typename = "bgm",
    .subdir = BGM_PATH_PREFIX,

    .procs = {
        .find = bgm_path,
        .check = check_bgm_path,
        .begin_load = load_bgm_begin,
        .end_load = load_bgm_end,
        .unload = unload_bgm,
    },
};
