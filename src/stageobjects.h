/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stageobjects_h
#define IGUARD_stageobjects_h

#include "taisei.h"

#include "objectpool.h"

typedef struct StageObjectPools {
	union {
		struct {
			ObjectPool *projectiles;    // includes particles as well
			ObjectPool *items;
			ObjectPool *enemies;
			ObjectPool *lasers;
			ObjectPool *stagetext;
			ObjectPool *bosses;
		};

		ObjectPool *first;
	};
} StageObjectPools;

extern StageObjectPools stage_object_pools;

void stage_objpools_alloc(void);
void stage_objpools_free(void);

#endif // IGUARD_stageobjects_h
