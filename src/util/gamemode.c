/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "gamemode.h"
#include "util.h"
#include "taskmanager.h"

#include <gamemode_client.h>

// NOTE: These dbus requests may block, so proxy them off to a background worker thread to not affect load times.

static void *gamemode_init_task(void *a) {
	if(gamemode_request_start() < 0) {
		log_error("gamemode_request_start() failed: %s", gamemode_error_string());
	}

	return NULL;
}

static void *gamemode_shutdown_task(void *a) {
	if(gamemode_request_end() < 0) {
		log_error("gamemode_request_end() failed: %s", gamemode_error_string());
	}

	return NULL;
}

static inline bool gamemode_control_enabled(void) {
	return env_get_int("TAISEI_GAMEMODE", true);
}

void gamemode_init(void) {
	if(gamemode_control_enabled()) {
		task_detach(taskmgr_global_submit((TaskParams) { .callback = gamemode_init_task }));
	}
}

void gamemode_shutdown(void) {
	if(gamemode_control_enabled()) {
		task_detach(taskmgr_global_submit((TaskParams) { .callback = gamemode_shutdown_task }));
	}
}
