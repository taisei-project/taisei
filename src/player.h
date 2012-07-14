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

#include "resource/animation.h"

enum {
	False = 0,
	True = 1
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
    
	int lifes;
	int bombs;
	
	int recovery;
	
	int deathtime;
    
    int continues;
    
	Character cha;
	ShotMode shot;
	Enemy *slaves;
	
	int moveflags;
} Player;

void init_player(Player*);
void player_draw(Player*);
void player_logic(Player*);

void plr_set_char(Player*, Character);
void plr_set_power(Player *plr, float npow);

void plr_move(Player*, complex delta);

void plr_bomb(Player*);
void plr_realdeath(Player*);
void plr_death(Player*);

#define MOVEFLAG_UP 1
#define MOVEFLAG_DOWN 2
#define MOVEFLAG_LEFT 4
#define MOVEFLAG_RIGHT 8

#define player_hasmoveflag(p,f) (((p).moveflags & (f)) == (f))
void player_setmoveflag(Player* plr, int key, int mode);
void player_event(Player* plr,int type, int key);
void player_applymovement(Player* plr);

#endif
