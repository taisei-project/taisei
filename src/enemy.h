/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "coroutine/coevent.h"
#include "entity.h"
#include "move.h"
#include "resource/resource.h"
#include "util.h"

#ifdef DEBUG
	#define ENEMY_DEBUG
#endif

#ifdef ENEMY_DEBUG
	#define IF_ENEMY_DEBUG(...) __VA_ARGS__
#else
	#define IF_ENEMY_DEBUG(...)
#endif

typedef LIST_ANCHOR(Enemy) EnemyList;

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

typedef struct EnemyDrawParams {
	cmplx pos;  // NOTE: subject to slide-in correction at screen edges
	int time;
} EnemyDrawParams;

typedef void (*EnemyDrawFunc)(Enemy*, EnemyDrawParams);

typedef struct EnemyVisual {
	EnemyDrawFunc draw;
	void *drawdata;
} EnemyVisual;

#define ENEMY_NOVISUAL ((EnemyVisual) {})

DEFINE_ENTITY_TYPE(Enemy, {
	cmplx pos;
	cmplx pos0;
	cmplx pos0_visual;
	MoveParams move;
	EnemyVisual visual;

	COEVENTS_ARRAY(
		predamage,
		damaged,
		killed
	) events;

	/*
	 * This field is usually NULL except during handling of "predamage", "damaged", and "killed"
	 * events.
	 *
	 * "Predamage" events are run any time something attempts to damage the enemy, before the
	 * damage is applied. The event handler may modify the DamageInfo struct to affect the outcome.
	 *
	 * "Damaged" events are run after the damage has been applied. Immunities are not considered
	 * successful applications. `damage_info` describes the damage that has been applied.
	 *
	 * If after the "damaged" event health drops to 0 or lower, the "killed" event is signaled and
	 * the enemy is marked for removal. `damage_info` describes the cause of death.
	 *
	 * The "damaged" event is also signaled when an enemy is killed through non-damage means, such
	 * as calling `enemy_kill()` directly. In that case, `damage_info` is NULL. This does NOT happen
	 * if the enemy is auto-removed due to going out of bounds - if you want to catch that case,
	 * watch for a cancelled "killed" event.
	 *
	 * `damage_info` is also NULL when handling a cancelled event of any type.
	 *
	 * The pointer becomes invalid as soon as the event handler yields.
	 */
	DamageInfo *damage_info;

	EnemyFlag flags;

	int birthtime;
	int dir;

	float spawn_hp;
	float hp;

	float hit_radius;
	float hurt_radius;

	float max_viewport_dist;

	bool moving;

	IF_ENEMY_DEBUG(
		DebugInfo debug;
	)
});

Enemy *create_enemy_p(EnemyList *enemies, cmplx pos, float hp, EnemyVisual visual);

#ifdef ENEMY_DEBUG
	Enemy *_enemy_attach_dbginfo(Enemy *p, DebugInfo *dbg);
	#define create_enemy_p(...) _enemy_attach_dbginfo(create_enemy_p(__VA_ARGS__), _DEBUG_INFO_PTR_)
#endif

#define create_enemy(pos, hp, visual) \
	create_enemy_p(&global.enemies, pos, hp, visual)

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

void enemies_preload(ResourceGroup *rg);
