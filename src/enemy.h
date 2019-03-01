/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_enemy_h
#define IGUARD_enemy_h

#include "taisei.h"

#include "util.h"
#include "projectile.h"
#include "objectpool.h"
#include "entity.h"

#ifdef DEBUG
	#define ENEMY_DEBUG
#endif

#define ENEMY_HURT_RADIUS 7

typedef struct Enemy Enemy;
typedef LIST_ANCHOR(Enemy) EnemyList;
typedef int (*EnemyLogicRule)(struct Enemy*, int t);
typedef void (*EnemyVisualRule)(struct Enemy*, int t, bool render);

enum {
	ENEMY_IMMUNE = -9000,
	ENEMY_BOMB = -9001,
	ENEMY_KILLED = -9002,
};

struct Enemy {
	ENTITY_INTERFACE_NAMED(Enemy, ent);

	complex pos;
	complex pos0;
	complex pos0_visual;

	long birthtime;

	int dir;
	bool moving;

	EnemyLogicRule logic_rule;
	EnemyVisualRule visual_rule;

	float spawn_hp;
	float hp;

	complex args[RULE_ARGC];
	float alpha;

#ifdef ENEMY_DEBUG
	DebugInfo debug;
#endif
};

#define create_enemy4c(p,h,d,l,a1,a2,a3,a4) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,a4)
#define create_enemy3c(p,h,d,l,a1,a2,a3) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,0)
#define create_enemy2c(p,h,d,l,a1,a2) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,0,0)
#define create_enemy1c(p,h,d,l,a1) create_enemy_p(&global.enemies,p,h,d,l,a1,0,0,0)

Enemy *create_enemy_p(
	EnemyList *enemies, complex pos, float hp, EnemyVisualRule draw_rule, EnemyLogicRule logic_rule,
	complex a1, complex a2, complex a3, complex a4
);

#ifdef ENEMY_DEBUG
	Enemy* _enemy_attach_dbginfo(Enemy *p, DebugInfo *dbg);
	#define create_enemy_p(...) _enemy_attach_dbginfo(create_enemy_p(__VA_ARGS__), _DEBUG_INFO_PTR_)
#endif

void delete_enemy(EnemyList *enemies, Enemy* enemy);
void delete_enemies(EnemyList *enemies);

void process_enemies(EnemyList *enemies);

bool enemy_is_vulnerable(Enemy *enemy);
bool enemy_in_viewport(Enemy *enemy);
void enemy_kill_all(EnemyList *enemies);

void Fairy(Enemy*, int t, bool render);
void Swirl(Enemy*, int t, bool render);
void BigFairy(Enemy*, int t, bool render);

int enemy_flare(Projectile *p, int t);

void enemies_preload(void);

#endif // IGUARD_enemy_h
