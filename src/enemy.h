/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SLAVE_H
#define SLAVE_H

#include "animation.h"
#include <complex.h>
#include <stdarg.h>

struct Enemy;
typedef void (*EnemyLogicRule)(struct Enemy*, int t);
typedef EnemyLogicRule EnemyDrawRule;

enum {
	ENEMY_IMMUNE = -9000,
};

typedef struct Enemy {
	struct Enemy *next;
	struct Enemy *prev;
	
	long birthtime;
	
	complex pos;
	complex pos0;
	
	int dir; // TODO: deprecate those
	int moving;
	
	EnemyLogicRule logic_rule;
	EnemyDrawRule draw_rule;
	
	int hp;
	
	void *parent;
	complex args[4];
} Enemy;

#define create_enemyg(drule, lrule, pos, hp, par, args) (create_enemy(&global.enemies, drule, lrule, pos, hp, par, args))
void create_enemy(Enemy **enemies, EnemyDrawRule draw_rule, EnemyLogicRule logic_rule,
				  complex pos, int hp, void *parent, complex args, ...);
void delete_enemy(Enemy **enemies, Enemy* enemy);
void draw_enemies(Enemy *enemies);
void free_enemies(Enemy **enemies);

void process_enemies(Enemy **enemies);

void Fairy(Enemy*, int t);
#endif