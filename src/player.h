/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef PLAYER_H
#define PLAYER_H

#ifdef DEBUG
	#define PLR_DPS_STATS
#endif

#include "util.h"
#include "enemy.h"
#include "gamepad.h"
#include "resource/animation.h"

enum {
	PLR_MAX_POWER = 400,
	PLR_MAX_LIVES = 9,
	PLR_MAX_BOMBS = 9,

	PLR_MAX_LIFE_FRAGMENTS = 5,
	PLR_MAX_BOMB_FRAGMENTS = 5,

	PLR_START_LIVES = 2,
	PLR_START_BOMBS = 3,

	PLR_SCORE_PER_LIFE_FRAG = 55000,
	PLR_SCORE_PER_BOMB_FRAG = 22000,

	PLR_SPELLPRACTICE_POWER = 200,

	PLR_STGPRACTICE_POWER = 200,
	PLR_STGPRACTICE_LIVES = 4,
	PLR_STGPRACTICE_BOMBS = 4,
};

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
	unsigned int points;

	int lives;
	int bombs;

	int life_fragments;
	int bomb_fragments;

	int recovery;

	int deathtime;
	int respawntime;

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

#ifdef PLR_DPS_STATS
	int total_dmg;
#endif
} Player;

// this is used by both player and replay code
enum {
	EV_PRESS,
	EV_RELEASE,
	EV_OVER, // replay-only
	EV_AXIS_LR,
	EV_AXIS_UD,
	EV_CHECK_DESYNC, // replay-only
	EV_FPS, // replay-only
};

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

void player_add_life_fragments(Player *plr, int frags);
void player_add_bomb_fragments(Player *plr, int frags);
void player_add_lives(Player *plr, int lives);
void player_add_bombs(Player *plr, int bombs);
void player_add_points(Player *plr, unsigned int points);

void player_preload(void);

#endif
