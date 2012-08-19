/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BOSS_H
#define BOSS_H

#include "tscomplex.h"

#include <resource/animation.h>
struct Boss;

typedef void (*BossRule)(struct Boss*, int time);

typedef enum AttackType {
	AT_Normal,
	AT_Move,
	AT_Spellcard,
	AT_SurvivalSpell,
	AT_ExtraSpell
} AttackType;

typedef struct AttackInfo {
	AttackType type;
	char *name;
	float timeout;
	int hp;

	BossRule rule;
	BossRule draw_rule;

	// complex pos_spawn;
	complex pos_dest;
} AttackInfo;

typedef struct Attack {
	char *name;

	AttackType type;

	int starttime;

	int timeout;
	int dmglimit;
	int finished;
	int endtime;

	BossRule rule;
	BossRule draw_rule;

	AttackInfo *info; // NULL for attacks created directly through boss_add_attack
} Attack;

enum {
	FINISH_NOPE,
	FINISH_WIN,
	FINISH_FAIL
};

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

Boss* create_boss(char *name, char *ani, complex pos);
void draw_extraspell_bg(Boss *boss, int time);
void draw_boss(Boss *boss);
void process_boss(Boss *boss);

void free_boss(Boss *boss);
void free_attack(Attack *a);

void start_attack(Boss *b, Attack *a);

Attack* boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule);
Attack* boss_add_attack_from_info(Boss *boss, AttackInfo *info, char move);

void boss_death(Boss **boss);
void boss_kill_projectiles(void);

#define BOSS_DEFAULT_SPAWN_POS (VIEWPORT_W * 0.5 - I * VIEWPORT_H * 0.5)
#define BOSS_DEFAULT_GO_POS (VIEWPORT_W * 0.5 + 200.0*I)
#define BOSS_NOMOVE (-3142-39942.0*I)

#endif
