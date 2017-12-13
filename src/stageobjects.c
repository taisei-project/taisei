
#include "stageobjects.h"
#include "projectile.h"
#include "item.h"
#include "enemy.h"
#include "laser.h"
#include "aniplayer.h"

#define MAX_PROJECTILES             8192
#define MAX_ITEMS                   MAX_PROJECTILES
#define MAX_ENEMIES                 64
#define MAX_LASERS                  256
#define MAX_ANIPLAYERS              32

StageObjectPools stage_object_pools;

void stage_objpools_alloc(void) {
    stage_object_pools.projectiles = objpool_alloc(sizeof(Projectile), MAX_PROJECTILES, "proj+part");
    stage_object_pools.items = objpool_alloc(sizeof(Item), MAX_ITEMS, "item");
    stage_object_pools.enemies = objpool_alloc(sizeof(Enemy), MAX_ENEMIES, "enemy");
    stage_object_pools.lasers = objpool_alloc(sizeof(Laser), MAX_LASERS, "laser");
    stage_object_pools.aniplayers = objpool_alloc(sizeof(AniPlayer), MAX_ANIPLAYERS, "aniplr");
}

void stage_objpools_free(void) {
    objpool_free(stage_object_pools.projectiles);
    objpool_free(stage_object_pools.items);
    objpool_free(stage_object_pools.enemies);
    objpool_free(stage_object_pools.lasers);
    objpool_free(stage_object_pools.aniplayers);
}
