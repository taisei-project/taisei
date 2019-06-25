/*
 * This software is licensed under the terms of the MIT-License
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
	return sfxbgm_make_path(RES_PATHPREFIX_MUSIC, name, true);
}

static bool check_bgm_path(const char *path) {
	return sfxbgm_check_path(RES_PATHPREFIX_MUSIC, path, true);
}

static MusicImpl *load_music(const char *path) {
	if(!path) {
		return NULL;
	}

	return _a_backend.funcs.music_load(path);
}

static void *load_bgm_begin(ResourceLoadInfo li) {
	Music *mus = calloc(1, sizeof(Music));
	MusicMetadata *meta = NULL;

	if(strendswith(li.path, ".bgm")) {
		mus->meta = res_ref(RES_MUSICMETA, li.name, li.flags | RESF_LAZY);
		meta = res_ref_data(mus->meta);
		mus->impl = load_music(meta->loop_path);
	} else {
		mus->impl = load_music(li.path);
	}

	if(!mus->impl) {
		free(mus);
		mus = NULL;
		log_error("Failed to load bgm '%s'", li.path);
	} else if(meta && meta->loop_point > 0) {
		_a_backend.funcs.music_set_loop_point(mus->impl, meta->loop_point);
	}

	return mus;
}

static void unload_bgm(void *vmus) {
	Music *mus = vmus;
	res_unref(&mus->meta, 1);
	_a_backend.funcs.music_unload(mus->impl);
	free(mus);
}

ResourceHandler bgm_res_handler = {
	.type = RES_MUSIC,

	.procs = {
		.find = bgm_path,
		.check = check_bgm_path,
		.begin_load = load_bgm_begin,
		.unload = unload_bgm,
	},
};
