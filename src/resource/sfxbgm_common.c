/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

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

char *sfxbgm_make_path(const char *prefix, const char *name, bool isbgm) {
	char *p = NULL;

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

	uint numexts;
	const char *const *exts = get_exts(isbgm, &numexts);

	for(uint i = 0; i < numexts; ++i) {
		if(strendswith(path, exts[i])) {
			return true;
		}
	}

	return false;
}
