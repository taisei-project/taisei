/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "sfx.h"

ResourceHandler sfx_res_handler = {
    .type = RES_SFX,
    .typename = "sfx",
    .subdir = SFX_PATH_PREFIX,

    .procs = {
        .find = sound_path,
        .check = check_sound_path,
        .begin_load = load_sound_begin,
        .end_load = load_sound_end,
        .unload = unload_sound,
    },
};
