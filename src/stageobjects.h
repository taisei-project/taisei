/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
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

#define OBJECT_POOLS \
	OBJECT_POOL(Projectile, projectiles) \
	OBJECT_POOL(Item,       items) \
	OBJECT_POOL(Enemy,      enemies) \
	OBJECT_POOL(Laser,      lasers) \
	OBJECT_POOL(StageText,  stagetext) \
	OBJECT_POOL(Boss,       bosses) \

typedef struct StageObjectPools {
	#define OBJECT_POOL(type, field) \
		MemPool field;

	OBJECT_POOLS
	#undef OBJECT_POOL
} StageObjectPools;

extern StageObjectPools stage_object_pools;

#define STAGE_OBJPOOLS_AS_ARRAYPTR \
	(MemPool (*)[sizeof(stage_object_pools) / sizeof(MemPool)])&stage_object_pools

// Can be called many times to reinitialize the pools while reusing allocated arena memory.
void stage_objpools_init(void);

// Frees the arena
void stage_objpools_shutdown(void);
