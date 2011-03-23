/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "texture.h"
#include "animation.h"
#include <complex.h>

enum {
	False = 0,
	True = 1
};

typedef enum {
	Youmu
} Character;

typedef struct {
	complex pos;
	short focus;
	short fire;
	short moving;	
	
	short dir;
	float power;
    
	int lives;
	int bombs;
	
	float recovery;
    
	Character cha;
	
	Animation *ani;
} Player;

void init_player(Player*, Character cha);

void player_draw(Player*);
void player_logic(Player*);

void plr_bomb();
#endif