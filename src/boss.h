/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BOSS_H
#define BOSS_H

#include <complex.h>

#include <resource/animation.h>
struct Boss;

typedef void (*BossRule)(struct Boss*, int time);

typedef enum AttackType {
	AT_Normal,
	AT_Move,
	AT_Spellcard,
	AT_SurvivalSpell
} AttackType;


typedef struct Attack {
	char *name;
	
	AttackType type;
	
	int starttime;
	
	int timeout;
	int dmglimit;
		
	BossRule rule;
	BossRule draw_rule;
} Attack;	

typedef struct Boss {	
	Attack *attacks;
	Attack *current;
	
	complex pos;
	
	char *name;
	
	int acount;
	
	Animation *ani;
	int anirow;
			
	int dmg;
	Color *zoomcolor;
} Boss;

Boss *create_boss(char *name, char *ani, complex pos);
void draw_boss(Boss *boss);
void process_boss(Boss *boss);

void free_boss(Boss *boss);
void free_attack(Attack *a);

void start_attack(Boss *b, Attack *a);

Attack *boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule);

void boss_death(Boss **boss);

#endif
