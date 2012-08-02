/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef ENEMY_H
#define ENEMY_H
#include <complex.h>
#include "projectile.h"

#undef complex
#define complex double _Complex

#include <stdarg.h>

typedef struct Enemy Enemy;
typedef int (*EnemyLogicRule)(struct Enemy*, int t);
typedef void (*EnemyDrawRule)(struct Enemy*, int t);

enum {
	ENEMY_IMMUNE = -9000,
	ENEMY_BOMB = -9001
};

struct Enemy {
	Enemy *next;
	Enemy *prev;
	
	complex pos;
	complex pos0;
	
	long birthtime;	
	
	int dir; // TODO: deprecate those
	int moving;
	
	EnemyLogicRule logic_rule;
	EnemyDrawRule draw_rule;
	
	int hp;
	int unbombable;
	
	complex args[RULE_ARGC];
	float alpha;
};

#define create_enemy4c(p,h,d,l,a1,a2,a3,a4) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,a4)
#define create_enemy3c(p,h,d,l,a1,a2,a3) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,a3,0)
#define create_enemy2c(p,h,d,l,a1,a2) create_enemy_p(&global.enemies,p,h,d,l,a1,a2,0,0)
#define create_enemy1c(p,h,d,l,a1) create_enemy_p(&global.enemies,p,h,d,l,a1,0,0,0)

Enemy *create_enemy_p(Enemy **enemies, complex pos, int hp, EnemyDrawRule draw_rule, EnemyLogicRule logic_rule,
				    complex a1, complex a2, complex a3, complex a4);
void delete_enemy(Enemy **enemies, Enemy* enemy);
void draw_enemies(Enemy *enemies);
void delete_enemies(Enemy **enemies);

void process_enemies(Enemy **enemies);

void killall(Enemy *enemies);

void Fairy(Enemy*, int t);
void Swirl(Enemy*, int t);
void BigFairy(Enemy*, int t);

int enemy_flare(Projectile *p, int t);
void EnemyFlareShrink(Projectile *p, int t);

#endif
