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

static void load_bgm(ResourceLoadState *st) {
	Music *mus = calloc(1, sizeof(Music));

	if(strendswith(st->path, ".bgm")) {
		mus->meta = get_resource_data(RES_BGM_METADATA, st->name, st->flags);

		if(mus->meta) {
			mus->impl = load_music(mus->meta->loop_path);
		}
	} else {
		mus->impl = load_music(st->path);
	}

	if(!mus->impl) {
		free(mus);
		mus = NULL;
		log_error("Failed to load bgm '%s'", st->path);
		res_load_failed(st);
	} else {
		if(mus->meta && mus->meta->loop_point > 0) {
			_a_backend.funcs.music_set_loop_point(mus->impl, mus->meta->loop_point);
		}

		res_load_finished(st, mus);
	}
}

static void unload_bgm(void *vmus) {
	Music *mus = vmus;
	_a_backend.funcs.music_unload(mus->impl);
	free(mus);
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
