/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "platform_paths.h"
#include "../public.h"

typedef struct tagLOGFONTW LOGFONTW;  /* broken piece of shit garbage windows headers */

#include <shlobj.h>
#include <objbase.h>
#include <SDL3/SDL.h>

const char *vfs_platformpath_cache(void) {
	static char *cached;

	if(!cached) {
		WCHAR *w_localappdata = NULL;

		if(
			SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &w_localappdata)) &&
			w_localappdata != NULL
		) {
			char *localappdata = SDL_iconv_string(
				"UTF-8", "UTF-16LE", (char*)w_localappdata, (SDL_wcslen(w_localappdata) + 1) * sizeof(WCHAR));
			CoTaskMemFree(w_localappdata);

			if(localappdata) {
				cached = vfs_syspath_join_alloc(localappdata, "taisei\\cache");
				SDL_free(localappdata);
			}
		}
	}

	return cached;
}
