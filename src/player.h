/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "texture.h"
#include "animation.h"

enum {
	False = 0,
	True = 1
};

typedef enum {
	Youmu
} Character;

typedef struct {
	float x, y;
	short focus;
	short fire;
	short moving;	
	
	short dir;
    float power;
    
	Character cha;
	
	Animation ani;
} Player;

void init_player(Player*, Character cha);

void player_draw(Player*);
void player_logic(Player*);
#endif