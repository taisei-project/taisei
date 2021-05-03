/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "enemy.h"

#include "global.h"
#include "projectile.h"
#include "list.h"
#include "aniplayer.h"
#include "stageobjects.h"
#include "util/glm.h"
#include "entity.h"

#ifdef create_enemy_p
#undef create_enemy_p
#endif

#ifdef DEBUG
Enemy *_enemy_attach_dbginfo(Enemy *e, DebugInfo *dbg) {
	memcpy(&e->debug, dbg, sizeof(DebugInfo));
	set_debug_info(dbg);
	return e;
}
#endif

static void ent_draw_enemy(EntityInterface *ent);
static DamageResult ent_damage_enemy(EntityInterface *ienemy, const DamageInfo *dmg);

static void fix_pos0_visual(Enemy *e) {
	if(e->flags & EFLAG_NO_VISUAL_CORRECTION) {
		return;
	}

	double x = creal(e->pos0_visual);
	double y = cimag(e->pos0_visual);
	double ofs = 21;

	if(x <= 0 && x > -ofs) {
		x = -ofs;
	} else if(x >= VIEWPORT_W && x < VIEWPORT_W + ofs) {
		x = VIEWPORT_W + ofs;
	}

	if(y <= 0 && y > -ofs) {
		y = -ofs;
	} else if(y >= VIEWPORT_H && y < VIEWPORT_H + ofs) {
		y = VIEWPORT_H + ofs;
	}

	e->pos0_visual = x + y * I;
}

static inline void _signal_event_with_damage_info(Enemy *e, CoEvent *evt, DamageInfo *dmg, void (*sigfunc)(CoEvent*)) {
	assert(e->damage_info == NULL);
	e->damage_info = dmg;
	sigfunc(evt);
	assert(e->damage_info == dmg);
	e->damage_info = NULL;
}

static void signal_event_with_damage_info(Enemy *e, CoEvent *evt, DamageInfo *dmg) {
	_signal_event_with_damage_info(e, evt, dmg, coevent_signal);
}

static void signal_event_once_with_damage_info(Enemy *e, CoEvent *evt, DamageInfo *dmg) {
	_signal_event_with_damage_info(e, evt, dmg, coevent_signal_once);
}

static inline int enemy_call_logic_rule(Enemy *e, int t) {
	assert(e->damage_info == NULL);

	if(t == EVENT_KILLED) {
		signal_event_once_with_damage_info(e, &e->events.killed, NULL);
	}

	if(e->logic_rule) {
		return e->logic_rule(e, t);
	} else if(t >= 0) {
		// TODO: backport unified left/right move animations from the obsolete `newart` branch
		cmplx v = move_update(&e->pos, &e->move);
		e->moving = fabs(creal(v)) >= 1;
		e->dir = creal(v) < 0;
	}

	return ACTION_NONE;
}

Enemy *create_enemy_p(EnemyList *enemies, cmplx pos, float hp, EnemyVisualRule visual_rule, EnemyLogicRule logic_rule,
				      cmplx a1, cmplx a2, cmplx a3, cmplx a4) {
	if(IN_DRAW_CODE) {
		log_fatal("Tried to spawn an enemy while in drawing code");
	}

	// FIXME: some code relies on the insertion logic (which?)
	Enemy *e = alist_insert(enemies, enemies->first, (Enemy*)objpool_acquire(stage_object_pools.enemies));
	// Enemy *e = alist_append(enemies, (Enemy*)objpool_acquire(stage_object_pools.enemies));
	e->moving = false;
	e->dir = 0;

	e->birthtime = global.frames;
	e->pos = pos;
	e->pos0 = pos;
	e->pos0_visual = pos;

	e->spawn_hp = hp;
	e->hp = hp;
	e->alpha = 1.0;

	e->flags = 0;

	if(e->hp == _internal_ENEMY_IMMUNE) {
		e->flags |= EFLAGS_GHOST;
	}

	e->logic_rule = logic_rule;
	e->visual_rule = visual_rule;

	e->args[0] = a1;
	e->args[1] = a2;
	e->args[2] = a3;
	e->args[3] = a4;

	e->hurt_radius = 7;
	e->hit_radius = 30;

	e->ent.draw_layer = LAYER_ENEMY;
	e->ent.draw_func = ent_draw_enemy;
	e->ent.damage_func = ent_damage_enemy;

	COEVENT_INIT_ARRAY(e->events);
	fix_pos0_visual(e);
	ent_register(&e->ent, ENT_TYPE_ID(Enemy));

	enemy_call_logic_rule(e, EVENT_BIRTH);
	return e;
}

static void enemy_death_effect(cmplx pos) {
	for(int i = 0; i < 10; i++) {
		RNG_ARRAY(rng, 2);
		PARTICLE(
			.sprite = "flare",
			.pos = pos,
			.timeout = 10,
			.draw_rule = pdraw_timeout_fade(1, 0),
			.move = move_linear(vrng_range(rng[0], 3, 13) * vrng_dir(rng[1])),
		);
	}

	PARTICLE(
		.proto = pp_blast,
		.pos = pos,
		.timeout = 20,
		.draw_rule = pdraw_blast(),
		.flags = PFLAG_REQUIREDPARTICLE
	);

	PARTICLE(
		.proto = pp_blast,
		.pos = pos,
		.timeout = 20,
		.draw_rule = pdraw_blast(),
		.flags = PFLAG_REQUIREDPARTICLE
	);

	PARTICLE(
		.proto = pp_blast,
		.pos = pos,
		.timeout = 15,
		.draw_rule = pdraw_timeout_scalefade(0, rng_f32_range(1, 2), 1, 0),
		.flags = PFLAG_REQUIREDPARTICLE
	);
}

static void *_delete_enemy(ListAnchor *enemies, List* enemy, void *arg) {
	Enemy *e = (Enemy*)enemy;

	if(e->hp <= 0 && !(e->flags & EFLAG_NO_DEATH_EXPLOSION)) {
		play_sfx("enemydeath");
		enemy_death_effect(e->pos);

		for(Projectile *p = global.projs.first; p; p = p->next) {
			if(p->type == PROJ_ENEMY && !(p->flags & PFLAG_NOCOLLISION) && cabs(p->pos - e->pos) < 64) {
				spawn_and_collect_item(e->pos, ITEM_PIV, 1);
			}
		}
	}

	COEVENT_CANCEL_ARRAY(e->events);
	enemy_call_logic_rule(e, EVENT_DEATH);
	ent_unregister(&e->ent);
	objpool_release(stage_object_pools.enemies, alist_unlink(enemies, enemy));

	return NULL;
}

void delete_enemy(EnemyList *enemies, Enemy* enemy) {
	_delete_enemy((ListAnchor*)enemies, (List*)enemy, NULL);
}

void delete_enemies(EnemyList *enemies) {
	alist_foreach(enemies, _delete_enemy, NULL);
}

cmplx enemy_visual_pos(Enemy *enemy) {
	if(enemy->flags & EFLAG_NO_VISUAL_CORRECTION) {
		return enemy->pos;
	}

	double t = (global.frames - enemy->birthtime) / 30.0;

	if(t >= 1) {
		return enemy->pos;
	}

	cmplx p = enemy->pos - enemy->pos0;
	p += t * enemy->pos0 + (1 - t) * enemy->pos0_visual;

	return p;
}

static void call_visual_rule(Enemy *e, bool render) {
	cmplx tmp = e->pos;
	e->pos = enemy_visual_pos(e);
	e->visual_rule(e, global.frames - e->birthtime, render);
	e->pos = tmp;
}

static void ent_draw_enemy(EntityInterface *ent) {
	Enemy *e = ENT_CAST(ent, Enemy);

	if(!e->visual_rule) {
		return;
	}

#ifdef ENEMY_DEBUG
	static Enemy prev_state;
	memcpy(&prev_state, e, sizeof(Enemy));
#endif

	call_visual_rule(e, true);

#ifdef ENEMY_DEBUG
	if(memcmp(&prev_state, e, sizeof(Enemy))) {
		set_debug_info(&e->debug);
		log_fatal("Enemy modified its own state in draw rule");
	}
#endif
}

int enemy_flare(Projectile *p, int t) { // a[0] velocity, a[1] ref to enemy
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return ACTION_ACK;
	}

	Enemy *owner = REF(p->args[1]);

	int result = ACTION_NONE;

	if(t == EVENT_BIRTH) {
		t = 0;
		result = ACTION_ACK;
	}

	if(owner != NULL) {
		p->args[3] = owner->pos;
	}

	p->pos = p->pos0 + p->args[3] + p->args[0]*t;
	return result;
}

bool enemy_is_vulnerable(Enemy *enemy) {
	return !(enemy->flags & EFLAG_INVULNERABLE);
}

bool enemy_is_targetable(Enemy *enemy) {
	return !(enemy->flags & EFLAG_NO_HIT);
}

bool enemy_in_viewport(Enemy *enemy) {
	double s = 60; // TODO: make this adjustable

	return
		creal(enemy->pos) >= -s &&
		creal(enemy->pos) <= VIEWPORT_W + s &&
		cimag(enemy->pos) >= -s &&
		cimag(enemy->pos) <= VIEWPORT_H + s;
}

void enemy_kill(Enemy *enemy) {
	signal_event_once_with_damage_info(enemy, &enemy->events.killed, NULL);
	enemy->flags |= EFLAG_KILLED | EFLAG_NO_HIT | EFLAG_NO_HURT | EFLAG_INVULNERABLE;
	enemy->hp = 0;
}

void enemy_kill_all(EnemyList *enemies) {
	for(Enemy *e = enemies->first; e; e = e->next) {
		enemy_kill(e);
	}
}

static DamageResult ent_damage_enemy(EntityInterface *ienemy, const DamageInfo *dmg) {
	Enemy *enemy = ENT_CAST(ienemy, Enemy);

	if(UNLIKELY(enemy->flags & EFLAG_KILLED)) {
		return DMG_RESULT_INAPPLICABLE;
	}

	DamageInfo ndmg = *dmg;
	signal_event_with_damage_info(enemy, &enemy->events.predamage, &ndmg);

	if(
		!enemy_is_vulnerable(enemy) ||
		ndmg.type == DMG_ENEMY_SHOT ||
		ndmg.type == DMG_ENEMY_COLLISION
	) {
		return DMG_RESULT_IMMUNE;
	}

	enemy->hp -= ndmg.amount;
	signal_event_with_damage_info(enemy, &enemy->events.damaged, &ndmg);

	if(enemy->hp <= 0) {
		signal_event_once_with_damage_info(enemy, &enemy->events.killed, &ndmg);
		enemy_kill(enemy);

		if(ndmg.type == DMG_PLAYER_DISCHARGE) {
			spawn_and_collect_items(enemy->pos, 1, ITEM_VOLTAGE, (int)imax(1, enemy->spawn_hp / 100));
		}
	}

	if(enemy->hp < enemy->spawn_hp * 0.1) {
		play_sfx_loop("hit1");
	} else {
		play_sfx_loop("hit0");
	}

	return DMG_RESULT_OK;
}

float enemy_get_hurt_radius(Enemy *enemy) {
	if(enemy->flags & EFLAG_NO_HURT || enemy->alpha < 1.0f) {
		return 0;
	} else {
		return enemy->hurt_radius;
	}
}

static bool should_auto_kill(Enemy *enemy) {
	return
		(enemy->hp <= 0 && enemy->hp != _internal_ENEMY_IMMUNE) ||
		(!(enemy->flags & EFLAG_NO_AUTOKILL) && !enemy_in_viewport(enemy));
}

void process_enemies(EnemyList *enemies) {
	for(Enemy *enemy = enemies->first, *next; enemy; enemy = next) {
		next = enemy->next;

		if(enemy->flags & EFLAG_KILLED) {
			enemy_call_logic_rule(enemy, EVENT_KILLED);
			delete_enemy(enemies, enemy);
			continue;
		}

		int action = enemy_call_logic_rule(enemy, global.frames - enemy->birthtime);

		float hurt_radius = enemy_get_hurt_radius(enemy);

		if(hurt_radius > 0 && cabs(enemy->pos - global.plr.pos) < hurt_radius) {
			ent_damage(&global.plr.ent, &(DamageInfo) { .type = DMG_ENEMY_COLLISION });
		}

		enemy->alpha = approach(enemy->alpha, 1.0, 1.0/60.0);

		if(action == ACTION_DESTROY || should_auto_kill(enemy)) {
			delete_enemy(enemies, enemy);
			continue;
		}

		if(enemy->visual_rule) {
			call_visual_rule(enemy, false);
		}
	}
}

void enemies_preload(void) {
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"enemy/fairy_blue",
		"enemy/fairy_red",
		"enemy/bigfairy",
		"enemy/hugefairy",
		"enemy/superfairy",
	NULL);

	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"fairy_circle",
		"fairy_circle_red",
		"fairy_circle_big_and_mean",
		"enemy/swirl",
	NULL);

	preload_resources(RES_SFX, RESF_OPTIONAL,
		"enemydeath",
	NULL);
}
