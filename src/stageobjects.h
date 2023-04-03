/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "objectpool.h"

#define OBJECT_POOLS \
	OBJECT_POOL(Projectile, projectiles) \
	OBJECT_POOL(Item,       items) \
	OBJECT_POOL(Enemy,      enemies) \
	OBJECT_POOL(Laser,      lasers) \
	OBJECT_POOL(StageText,  stagetext) \
	OBJECT_POOL(Boss,       bosses) \

typedef struct StageObjectPools {
	#define OBJECT_POOL(type, field) \
		ObjectPool field;

	OBJECT_POOLS
	#undef OBJECT_POOL
} StageObjectPools;

extern StageObjectPools stage_object_pools;

#define STAGE_OBJPOOLS_AS_ARRAYPTR \
	(ObjectPool (*)[sizeof(stage_object_pools) / sizeof(ObjectPool)])&stage_object_pools

// Can be called many times to reinitialize the pools while reusing allocated arena memory.
void stage_objpools_init(void);

// Frees the arena
void stage_objpools_shutdown(void);
