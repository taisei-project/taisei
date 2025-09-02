/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stageobjects.h"

#define INIT_ARENA_SIZE (8 << 20)

StageObjects stage_objects;

void stage_objpools_init(void) {
	if(!stage_objects.arena.pages.first) {
		marena_init(&stage_objects.arena, INIT_ARENA_SIZE - sizeof(MemArenaPage));
	} else {
		marena_reset(&stage_objects.arena);
	}

	stage_objects.pools = (typeof(stage_objects.pools)) {};
}

void stage_objpools_shutdown(void) {
	marena_deinit(&stage_objects.arena);
}
