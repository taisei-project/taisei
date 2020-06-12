/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "resource.h"
#include "bgm.h"
#include "audio/backend.h"
#include "sfxbgm_common.h"
#include "util.h"

static char *bgm_path(const char *name) {
	return sfxbgm_make_path(BGM_PATH_PREFIX, name, true, true);
}

static bool check_bgm_path(const char *path) {
	return sfxbgm_check_path(BGM_PATH_PREFIX, path, true);
}

static BGM *try_load(const char *path) {
	if(!path) {
		return NULL;
	}

	return _a_backend.funcs.bgm_load(path);
}

static void load_bgm(ResourceLoadState *st) {
	BGM *bgm = NULL;
	double loop_point = 0;

	if(strendswith(st->path, ".bgm")) {
		char *loop_path = NULL;

		if(!parse_keyvalue_file_with_spec(st->path, (KVSpec[]) {
			{ "loop",       .out_str    = &loop_path  },
			{ "loop_point", .out_double = &loop_point },
			{ NULL }
		})) {
			log_error("Failed to parse BGM metadata '%s'", st->path);
			free(loop_path);
			res_load_failed(st);
			return;
		}

		if(!loop_path) {
			loop_path = sfxbgm_make_path(BGM_PATH_PREFIX, st->name, true, false);
		}

		if(!loop_path) {
			log_error("Couldn't determine source path for BGM '%s'", st->name);
			res_load_failed(st);
			return;
		}

		bgm = try_load(loop_path);
		free(loop_path);
	} else {
		bgm = try_load(st->path);
	}

	if(bgm) {
		_a_backend.funcs.bgm_set_loop_point(bgm, loop_point);
		res_load_finished(st, bgm);
	} else {
		log_error("Failed to load bgm '%s'", st->path);
		res_load_failed(st);
	}
}

static void unload_bgm(void *vbgm) {
	_a_backend.funcs.bgm_unload(vbgm);
}

const char *bgm_get_title(BGM *bgm) {
	return _a_backend.funcs.object.bgm.get_title(bgm);
}

const char *bgm_get_artist(BGM *bgm) {
	return _a_backend.funcs.object.bgm.get_artist(bgm);
}

const char *bgm_get_comment(BGM *bgm) {
	return _a_backend.funcs.object.bgm.get_comment(bgm);
}

double bgm_get_duration(BGM *bgm) {
	return _a_backend.funcs.object.bgm.get_duration(bgm);
}

double bgm_get_loop_start(BGM *bgm) {
	return _a_backend.funcs.object.bgm.get_loop_start(bgm);
}

ResourceHandler bgm_res_handler = {
    .type = RES_BGM,
    .typename = "bgm",
    .subdir = BGM_PATH_PREFIX,

    .procs = {
        .find = bgm_path,
        .check = check_bgm_path,
        .load = load_bgm,
        .unload = unload_bgm,
    },
};
