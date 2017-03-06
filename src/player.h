/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include "tscomplex.h"
#include "enemy.h"
#include "gamepad.h"
#include "resource/animation.h"

typedef enum {
	// do not reorder these or you'll break replays

	INFLAG_UP = 1,
	INFLAG_DOWN = 2,
	INFLAG_LEFT = 4,
	INFLAG_RIGHT = 8,
	INFLAG_FOCUS = 16,
	INFLAG_SHOT = 32,
} PlrInputFlag;

enum {
	INFLAGS_MOVE = INFLAG_UP | INFLAG_DOWN | INFLAG_LEFT | INFLAG_RIGHT
};

typedef enum {
	Youmu = 0,
	Marisa
} Character;

typedef enum {
	YoumuOpposite = 0,
	YoumuHoming,

	MarisaLaser = YoumuOpposite,
	MarisaStar = YoumuHoming
} ShotMode;

typedef struct {
	complex pos;
	short focus;
	bool moving;

	short dir;
	short power;

	int graze;
	int points;

	int lifes;
	int bombs;

	int recovery;

	int deathtime;
	int respawntime;

    int continues;

	Character cha;
	ShotMode shot;
	Enemy *slaves;

	int inputflags;
	int curmove;
	int movetime;
	int prevmove;
	int prevmovetime;
	int gamepadmove;

	int axis_ud;
	int axis_lr;

	char iddqd;
} Player;

void init_player(Player*);
void prepare_player_for_next_stage(Player*);

void player_draw(Player*);
void player_logic(Player*);

void player_set_char(Player*, Character);
void player_set_power(Player *plr, short npow);

void player_move(Player*, complex delta);

void player_bomb(Player*);
void player_realdeath(Player*);
void player_death(Player*);
void player_graze(Player*, complex, int);

void player_setinputflag(Player *plr, KeyIndex key, bool mode);
void player_event(Player* plr, int type, int key);
void player_applymovement(Player* plr);
void player_input_workaround(Player *plr);

#endif
