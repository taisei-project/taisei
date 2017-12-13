
#pragma once

#include "objectpool.h"

typedef struct StageObjectPools {
    union {
        struct {
            ObjectPool *projectiles;    // includes particles as well
            ObjectPool *items;
            ObjectPool *enemies;
            ObjectPool *lasers;
            ObjectPool *aniplayers;     // hack for the boss glow effect
        };

        ObjectPool *first;
    };
} StageObjectPools;

extern StageObjectPools stage_object_pools;

void stage_objpools_alloc(void);
void stage_objpools_free(void);
