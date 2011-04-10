/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <complex.h>
#include "texture.h"
#include "animation.h"
#include "slave.h"

enum {
	False = 0,
	True = 1
};

typedef enum {
	Youmu,
	Marisa
} Character;

typedef enum {
	YoumuOpposite
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
	
	float recovery;
    
	Character cha;
	ShotMode shot;
	Slave *slaves;
	
	Animation *ani;
} Player;

void init_player(Player*, Character cha);

void player_draw(Player*);
void player_logic(Player*);

void plr_bomb();
#endif