/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BOSS_H
#define BOSS_H

#include "animation.h"
#include <complex.h>

struct Boss;

typedef void (*BossRule)(struct Boss*, int time);

typedef enum AttackType {
	Normal,
	Spellcard,
	SurvivalSpell
} AttackType;


typedef struct Attack {
	char *name;
	
	AttackType type;
	
	int starttime;
	
	int timeout;
	int dmglimit;
	
	struct Waypoint {
		complex pos;
		int time;
	}* waypoints;
	int wpcount;
	int wp;
	
	BossRule rule;
} Attack;	

typedef struct Boss {	
	char *name;
	
	Attack *attacks;
	Attack *current;
	int acount;
	
	Animation *ani;
	int anirow;
	
	complex pos;
	complex pos0;
	int time0;
	
	int dmg;
} Boss;

Boss *create_boss(char *name, char *ani, complex pos);
void draw_boss(Boss *boss);
void process_boss(Boss *boss);

void free_boss(Boss *boss);
void free_attack(Attack *a);

void start_attack(Boss *b, Attack *a);

Attack *boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule);
void boss_add_waypoint(Attack *attack, complex pos, int time);

#endif