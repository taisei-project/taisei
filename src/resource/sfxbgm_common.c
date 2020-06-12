/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sfxbgm_common.h"
#include "audio/backend.h"
#include "util.h"

static const char *const *get_exts(bool isbgm, uint *numexts) {
	const char *const *exts = (isbgm
		? _a_backend.funcs.get_supported_bgm_exts(numexts)
		: _a_backend.funcs.get_supported_sfx_exts(numexts)
	);

	if(!exts) {
		*numexts = 0;
	}

	return exts;
}

char *sfxbgm_make_path(const char *prefix, const char *name, bool isbgm, bool trymeta) {
	char *p = NULL;

	if(isbgm && trymeta && (p = try_path(prefix, name, ".bgm"))) {
		return p;
	}

	uint numexts;
	const char *const *exts = get_exts(isbgm, &numexts);

	for(uint i = 0; i < numexts; ++i) {
		if((p = try_path(prefix, name, exts[i]))) {
			return p;
		}
	}

	return NULL;
}

bool sfxbgm_check_path(const char *prefix, const char *path, bool isbgm) {
	if(prefix && !strstartswith(path, prefix)) {
		return false;
	}

	if(isbgm && strendswith(path, ".bgm")) {
		return true;
	}

	uint numexts;
	const char *const *exts = get_exts(isbgm, &numexts);

	for(uint i = 0; i < numexts; ++i) {
		if(strendswith(path, exts[i])) {
			return true;
		}
	}

	return false;
}
