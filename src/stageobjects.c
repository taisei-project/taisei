/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stageobjects.h"
#include "projectile.h"
#include "item.h"
#include "enemy.h"
#include "laser.h"
#include "stagetext.h"
#include "boss.h"
#include "aniplayer.h"

#define INIT_ARENA_SIZE (8 << 20)

StageObjectPools stage_object_pools;

static struct {
	MemArena arena;
} stgobjs;

void stage_objpools_init(void) {
	if(!stgobjs.arena.pages.first) {
		marena_init(&stgobjs.arena, INIT_ARENA_SIZE - sizeof(MemArenaPage));
	} else {
		marena_reset(&stgobjs.arena);
	}

	#define OBJECT_POOL(type, field) \
		objpool_init( \
			&stage_object_pools.field, \
			#type, \
			&stgobjs.arena, \
			sizeof(type), \
			alignof(type));

		OBJECT_POOLS
	#undef OBJECT_POOL
}

void stage_objpools_shutdown(void) {
	marena_deinit(&stgobjs.arena);
}
