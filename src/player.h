/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <complex.h>
#include "enemy.h"
#include "gamepad.h"
#include "resource/animation.h"

enum {
	False = 0,
	True = 1,
	
	MOVEFLAG_UP = 1,
	MOVEFLAG_DOWN = 2,
	MOVEFLAG_LEFT = 4,
	MOVEFLAG_RIGHT = 8
};

typedef enum {
	Youmu,
	Marisa
} Character;

typedef enum {
	YoumuOpposite,
	YoumuHoming,
	
	MarisaLaser = YoumuOpposite,
	MarisaStar = YoumuHoming
} ShotMode;

typedef struct {
	complex pos;
	short focus;
	short fire;
	short moving;
	
	short dir;
	float power;
	int graze;
    
	int lifes;
	int bombs;
	
	int recovery;
	
	int deathtime;
	int respawntime;
    
    int continues;
    
	Character cha;
	ShotMode shot;
	Enemy *slaves;
	
	int moveflags;
	int curmove;
	int movetime;
	int prevmove;
	int prevmovetime;
	
	int axis_ud;
	int axis_lr;
} Player;

void init_player(Player*);
void player_draw(Player*);
void player_logic(Player*);

void player_set_char(Player*, Character);
void player_set_power(Player *plr, float npow);

void player_move(Player*, complex delta);

void player_bomb(Player*);
void player_realdeath(Player*);
void player_death(Player*);
void player_graze(Player*, complex, int);

void player_setmoveflag(Player* plr, int key, int mode);
void player_event(Player* plr,int type, int key);
void player_applymovement(Player* plr);
void player_input_workaround(Player *plr);

#endif
