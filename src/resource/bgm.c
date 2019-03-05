/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "resource.h"
#include "bgm.h"
#include "audio/backend.h"
#include "sfxbgm_common.h"
#include "util.h"

static char* bgm_path(const char *name) {
	return sfxbgm_make_path(BGM_PATH_PREFIX, name, true);
}

static bool check_bgm_path(const char *path) {
	return sfxbgm_check_path(BGM_PATH_PREFIX, path, true);
}

static MusicImpl *load_music(const char *path) {
	if(!path) {
		return NULL;
	}

	return _a_backend.funcs.music_load(path);
}

static void *load_bgm_begin(const char *path, uint flags) {
	Music *mus = calloc(1, sizeof(Music));
	double loop_point = 0;

	if(strendswith(path, ".bgm")) {
		char *intro = NULL;
		char *loop = NULL;

		if(!parse_keyvalue_file_with_spec(path, (KVSpec[]) {
			{ "artist" }, // don’t print a warning because this field is supposed to be here
			{ "title",      .out_str    = &mus->title },
			{ "loop",       .out_str    = &loop       },
			{ "loop_point", .out_double = &loop_point },
			{ NULL }
		})) {
			log_error("Failed to parse bgm config '%s'", path);
		} else {
			mus->impl = load_music(loop);
		}

		free(intro);
		free(loop);
	} else {
		mus->impl = load_music(path);
	}

	if(!mus->impl) {
		free(mus->title);
		free(mus);
		mus = NULL;
		log_error("Failed to load bgm '%s'", path);
	} else if(loop_point > 0) {
		_a_backend.funcs.music_set_loop_point(mus->impl, loop_point);
	}

	return mus;
}

static void *load_bgm_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

static void unload_bgm(void *vmus) {
	Music *mus = vmus;
	_a_backend.funcs.music_unload(mus->impl);
	free(mus->title);
	free(mus);
}

ResourceHandler bgm_res_handler = {
    .type = RES_BGM,
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
