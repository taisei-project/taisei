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

#define MAX_projectiles             2048
#define MAX_items                   MAX_projectiles
#define MAX_enemies                 64
#define MAX_lasers                  64
#define MAX_stagetext               1024
#define MAX_bosses                  1

#define OBJECT_POOLS \
	OBJECT_POOL(Projectile, projectiles) \
	OBJECT_POOL(Item, items) \
	OBJECT_POOL(Enemy, enemies) \
	OBJECT_POOL(Laser, lasers) \
	OBJECT_POOL(StageText, stagetext) \
	OBJECT_POOL(Boss, bosses) \

StageObjectPools stage_object_pools;

void stage_objpools_alloc(void) {
	stage_object_pools = (StageObjectPools){
		#define OBJECT_POOL(type,field) \
			.field = OBJPOOL_ALLOC(type, MAX_##field),

		OBJECT_POOLS
		#undef OBJECT_POOL
	};
}

void stage_objpools_free(void) {
	#define OBJECT_POOL(type,field) \
		objpool_free(stage_object_pools.field);

	OBJECT_POOLS
	#undef OBJECT_POOL
}
