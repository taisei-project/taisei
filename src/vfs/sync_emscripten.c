/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "public.h"

#include "log.h"

#include <emscripten.h>

void EMSCRIPTEN_KEEPALIVE vfs_sync_callback(bool is_load, char *error, CallChain *next);

void vfs_sync_callback(bool is_load, char *error, CallChain *next) {
	if(error) {
		if(is_load) {
			log_error("Couldn't load persistent storage from IndexedDB: %s", error);
		} else {
			log_error("Couldn't save persistent storage to IndexedDB: %s", error);
		}
	} else {
		if(is_load) {
			log_info("Loaded persistent storage from IndexedDB");
		} else {
			log_info("Saved persistent storage to IndexedDB");
		}
	}

	run_call_chain(next, error);
	free(next);
}

void vfs_sync(VFSSyncMode mode, CallChain next) {
	CallChain *cc = memdup(&next, sizeof(next));
	EM_ASM({
		SyncFS($0, $1);
	}, (mode == VFS_SYNC_LOAD), cc);
}
