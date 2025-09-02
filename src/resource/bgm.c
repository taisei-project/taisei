/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "bgm.h"

#include "resource.h"
#include "audio/backend.h"
#include "sfxbgm_common.h"

static char *bgm_path(const char *name) {
	return sfxbgm_make_path(BGM_PATH_PREFIX, name, true);
}

static bool check_bgm_path(const char *path) {
	return sfxbgm_check_path(BGM_PATH_PREFIX, path, true);
}

static void load_bgm(ResourceLoadState *st) {
	BGM *bgm = _a_backend.funcs.bgm_load(st->path);

	if(bgm) {
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
	if(!bgm) {
		return NULL;
	}

	return _a_backend.funcs.object.bgm.get_title(bgm);
}

const char *bgm_get_artist(BGM *bgm) {
	if(!bgm) {
		return NULL;
	}

	return _a_backend.funcs.object.bgm.get_artist(bgm);
}

const char *bgm_get_comment(BGM *bgm) {
	if(!bgm) {
		return NULL;
	}

	return _a_backend.funcs.object.bgm.get_comment(bgm);
}

double bgm_get_duration(BGM *bgm) {
	if(!bgm) {
		return -1;
	}

	return _a_backend.funcs.object.bgm.get_duration(bgm);
}

double bgm_get_loop_start(BGM *bgm) {
	if(!bgm) {
		return -1;
	}

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
