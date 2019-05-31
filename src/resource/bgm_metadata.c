/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "bgm_metadata.h"
#include "bgm.h"
#include "util.h"

static char *bgm_meta_path(const char *name) {
	return strjoin(RES_PATHPREFIX_MUSICMETA, name, ".bgm", NULL);
}

static bool check_bgm_meta_path(const char *path) {
	return strendswith(path, ".bgm") && strstartswith(path, RES_PATHPREFIX_MUSICMETA);
}

static void free_metadata_fields(MusicMetadata *meta) {
	free(meta->artist);
	free(meta->comment);
	free(meta->loop_path);
	free(meta->title);
}

static void *load_bgm_meta_begin(ResourceLoadInfo li) {
	MusicMetadata meta = { 0 };

	if(!parse_keyvalue_file_with_spec(li.path, (KVSpec[]) {
		{ "artist",     .out_str    = &meta.artist     },
		{ "title",      .out_str    = &meta.title      },
		{ "comment",    .out_str    = &meta.comment    },
		{ "loop",       .out_str    = &meta.loop_path  },
		{ "loop_point", .out_double = &meta.loop_point },
		{ NULL }
	})) {
		log_error("Failed to parse BGM metadata '%s'", li.path);
		free_metadata_fields(&meta);
		return NULL;
	}

	if(meta.comment) {
		expand_escape_sequences(meta.comment);
	}

	return memdup(&meta, sizeof(meta));
}

static void unload_bgm_meta(void *vmus) {
	MusicMetadata *meta = vmus;
	free_metadata_fields(meta);
	free(meta);
}

ResourceHandler bgm_metadata_res_handler = {
    .type = RES_MUSICMETA,

    .procs = {
        .find = bgm_meta_path,
        .check = check_bgm_meta_path,
        .begin_load = load_bgm_meta_begin,
        .unload = unload_bgm_meta,
    },
};
