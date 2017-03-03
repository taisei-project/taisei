/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BOSS_H
#define BOSS_H

#include "tscomplex.h"
#include "difficulty.h"
#include "resource/animation.h"
#include "color.h"

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
	/*
		HOW TO SET UP THE IDMAP FOR DUMMIES

		The idmap is used as a template to generate spellstage IDs in stage_init_array().
		It looks like this:

			{id_easy, id_normal, id_hard, id_lunatic}

		Where each of those IDs are in range of 0-127, and are unique within each stage-specific AttackInfo array.
		A special value of -1 can be used to not generate a spellstage for specific difficulties.
		This is useful if your spell is only available on Hard and Lunatic, or has a difficulty-dependent name, etc.

		IDs of AT_ExtraSpell attacks may overlap with those of other types.

		Most importantly DO NOT CHANGE ENSTABILISHED IDs unless you absolutely must.
		Doing so is going to break replays, progress files, and anything that stores stage IDs permanently.
		Stage IDs are an internal detail invisible to the player, so they don't need to have any kind of fancy ordering.
	*/
	signed char idmap[NUM_SELECTABLE_DIFFICULTIES];

	AttackType type;
	char *name;
	float timeout;
	int hp;

	BossRule rule;
	BossRule draw_rule;

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
	Color zoomcolor;
} Boss;

Boss* create_boss(char *name, char *ani, complex pos);
void draw_extraspell_bg(Boss *boss, int time);
void draw_boss(Boss *boss);
void process_boss(Boss **boss);

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
