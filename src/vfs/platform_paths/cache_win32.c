/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "platform_paths.h"
#include "../public.h"

#include "memory/scratch.h"
#include "util/strbuf.h"
#include "util/stringops.h"

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
			StringBuffer buf = { acquire_scratch_arena() };
			utf16_to_utf8(SDL_wcslen(w_localappdata), w_localappdata, &buf);
			CoTaskMemFree(w_localappdata);
			cached = vfs_syspath_join_alloc(buf.start, "taisei\\cache");
			release_scratch_arena(buf.arena);
		}
	}

	return cached;
}
