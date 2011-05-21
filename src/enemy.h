/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef ENEMY_H
#define ENEMY_H

#include "animation.h"
#include <complex.h>
#include "projectile.h"

#undef complex
#define complex double _Complex

#include <stdarg.h>

struct Enemy;
typedef void (*EnemyLogicRule)(struct Enemy*, int t);
typedef EnemyLogicRule EnemyDrawRule;

enum {
	ENEMY_IMMUNE = -9000
};

typedef struct Enemy {
	struct Enemy *next;
	struct Enemy *prev;
	
	complex pos;
	complex pos0;
	
	long birthtime;	
	
	int dir; // TODO: deprecate those
	int moving;
	
	EnemyLogicRule logic_rule;
	EnemyDrawRule draw_rule;
	
	int hp;
	
	void *parent;
	complex args[RULE_ARGC];
} Enemy;

#define create_enemyg(drule, lrule, pos, hp, par, args) (create_enemy(&global.enemies, drule, lrule, pos, hp, par, args))
void create_enemy(Enemy **enemies, EnemyDrawRule draw_rule, EnemyLogicRule logic_rule,
				  complex pos, int hp, void *parent, complex args, ...);
void delete_enemy(Enemy **enemies, Enemy* enemy);
void draw_enemies(Enemy *enemies);
void delete_enemies(Enemy **enemies);

void process_enemies(Enemy **enemies);

void Fairy(Enemy*, int t);
#endif