/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/mempool.h"
#include "aniplayer.h"  // IWYU pragma: export
#include "projectile.h"  // IWYU pragma: export
#include "item.h"  // IWYU pragma: export
#include "enemy.h"  // IWYU pragma: export
#include "lasers/laser.h"  // IWYU pragma: export
#include "stagetext.h"  // IWYU pragma: export
#include "boss.h"  // IWYU pragma: export

#define OBJECT_POOLS(X) \
	X(Projectile, projectiles) \
	X(Item,       items) \
	X(Enemy,      enemies) \
	X(Laser,      lasers) \
	X(StageText,  stagetext) \
	X(Boss,       bosses) \

enum {
	#define COUNT_POOL(...) 1 +
	NUM_STAGE_OBJECT_POOLS = OBJECT_POOLS(COUNT_POOL) 0,
	#undef COUNT_POOL
};

typedef struct StageObjects {
	MemArena arena;
	union {
		struct {
			#define DECLARE_POOL(type, field) \
				MEMPOOL(type) field;

			OBJECT_POOLS(DECLARE_POOL)
			#undef DECLARE_POOL
		};
		MemPool as_array[NUM_STAGE_OBJECT_POOLS];
	} pools;
} StageObjects;

extern StageObjects stage_objects;

#define STAGE_OBJPOOL_GENERIC_DISPATCH(_type, _field) \
	_type*: &stage_objects.pools._field,

#define STAGE_OBJPOOL_BY_VARTYPE(_p_obj) \
	_Generic((_p_obj),  \
		OBJECT_POOLS(STAGE_OBJPOOL_GENERIC_DISPATCH) \
		struct {}: abort())

#define STAGE_OBJPOOL_BY_TYPE(type) \
	STAGE_OBJPOOL_BY_VARTYPE(&(type) {})

// Can be called many times to reinitialize the pools while reusing allocated arena memory.
void stage_objpools_init(void);

// Frees the arena
void stage_objpools_shutdown(void);

#define STAGE_ACQUIRE_OBJ(_type) \
	mempool_acquire(STAGE_OBJPOOL_BY_TYPE(_type), &stage_objects.arena)

#define STAGE_RELEASE_OBJ(_p_obj) \
	mempool_release(STAGE_OBJPOOL_BY_VARTYPE(_p_obj), (_p_obj))
