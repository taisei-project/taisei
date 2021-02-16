/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_enemy_h
#define IGUARD_enemy_h

#include "taisei.h"

#include "util.h"
#include "projectile.h"
#include "objectpool.h"
#include "entity.h"
#include "coroutine.h"
#include "move.h"

#ifdef DEBUG
	#define ENEMY_DEBUG
#endif

#ifdef ENEMY_DEBUG
	#define IF_ENEMY_DEBUG(...) __VA_ARGS__
#else
	#define IF_ENEMY_DEBUG(...)
#endif

typedef LIST_ANCHOR(Enemy) EnemyList;
typedef int (*EnemyLogicRule)(Enemy*, int t);
typedef void (*EnemyVisualRule)(Enemy*, int t, bool render);

typedef enum EnemyFlag {
	EFLAG_KILLED                = (1 << 0),  // is dead, pending removal (internal)
	EFLAG_NO_HIT                = (1 << 1),  // can't be hit by player
	EFLAG_NO_HURT               = (1 << 2),  // can't hurt player
	EFLAG_NO_AUTOKILL           = (1 << 3),  // no autokill when out of bounds
	EFLAG_NO_VISUAL_CORRECTION  = (1 << 4),  // disable the slide-in hack for enemies spawning at screen edges
	EFLAG_NO_DEATH_EXPLOSION    = (1 << 5),  // don't explode on death; currently also disables bonus voltage on kill
	EFLAG_INVULNERABLE          = (1 << 6),  // can't be damaged by player (but can be hit)
	EFLAG_IMPENETRABLE          = (1 << 7),  // penetrating shots can't pass through (e.g. Marisa's laser)

	EFLAGS_GHOST =
		EFLAG_NO_HIT |
		EFLAG_NO_HURT |
		EFLAG_NO_AUTOKILL |
		EFLAG_NO_VISUAL_CORRECTION |
		EFLAG_NO_DEATH_EXPLOSION |
		EFLAG_INVULNERABLE |
		0,
} EnemyFlag;

enum {
	_internal_ENEMY_IMMUNE = -9000,
	ENEMY_IMMUNE attr_deprecated("Set enemy flags explicitly") = _internal_ENEMY_IMMUNE,
// 	ENEMY_KILLED = -9002,
};

DEFINE_ENTITY_TYPE(Enemy, {
	cmplx pos;
	cmplx pos0;
	cmplx pos0_visual;

	union {
		cmplx args[RULE_ARGC];
		MoveParams move;
	};

	EnemyLogicRule logic_rule;
	EnemyVisualRule visual_rule;

	struct {
		CoEvent killed;
	} events;

	EnemyFlag flags;

	int birthtime;
	int dir;
	int next_graze;

	float spawn_hp;
	float hp;

	float alpha;

	float hit_radius;
	float hurt_radius;

	bool moving;

	IF_ENEMY_DEBUG(
		DebugInfo debug;
	)
});

#define create_enemy4c(p,h,d,l,a1,a2,a3,a4) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,a4)
#define create_enemy3c(p,h,d,l,a1,a2,a3) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,0)
#define create_enemy2c(p,h,d,l,a1,a2) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,0,0)
#define create_enemy1c(p,h,d,l,a1) create_enemy_p(&global.enemies,p,h,d,l,a1,0,0,0)

Enemy *create_enemy_p(
	EnemyList *enemies, cmplx pos, float hp, EnemyVisualRule draw_rule, EnemyLogicRule logic_rule,
	cmplx a1, cmplx a2, cmplx a3, cmplx a4
);

#ifdef ENEMY_DEBUG
	Enemy *_enemy_attach_dbginfo(Enemy *p, DebugInfo *dbg);
	#define create_enemy_p(...) _enemy_attach_dbginfo(create_enemy_p(__VA_ARGS__), _DEBUG_INFO_PTR_)
#endif

void delete_enemy(EnemyList *enemies, Enemy* enemy);
void delete_enemies(EnemyList *enemies);

void process_enemies(EnemyList *enemies);

bool enemy_is_vulnerable(Enemy *enemy);
bool enemy_is_targetable(Enemy *enemy);
bool enemy_in_viewport(Enemy *enemy);
float enemy_get_hurt_radius(Enemy *enemy);
cmplx enemy_visual_pos(Enemy *enemy);

void enemy_kill(Enemy *enemy);
void enemy_kill_all(EnemyList *enemies);

void Fairy(Enemy*, int t, bool render) attr_deprecated("Use the espawn_ API from enemy_classes.h");
void Swirl(Enemy*, int t, bool render) attr_deprecated("Use the espawn_ API from enemy_classes.h");
void BigFairy(Enemy*, int t, bool render) attr_deprecated("Use the espawn_ API from enemy_classes.h");

int enemy_flare(Projectile *p, int t) attr_deprecated("Use tasks");

void enemies_preload(void);

#endif // IGUARD_enemy_h
