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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL/SDL.h>
#include "player.h"
#include "projectile.h"
#include "fairy.h"

enum {
	RESX = 800,
	RESY = 600,
	
	SCREEN_W = 800,
	SCREEN_H = 600,
	
	VIEWPORT_X = 40,
	VIEWPORT_Y = 20,
	VIEWPORT_W = 480,
	VIEWPORT_H = 560,
	
	FPS = 60
};

#define FILE_PREFIX PREFIX "/share/openth/"

typedef struct {
	Player plr;	
	Projectile *projs;
	Fairy *fairies;
	
	int frames;		
	
	struct {
		Texture hud;
		
		Texture projwave;
		Texture water;
		Texture fairy_circle;
		Texture focus;
		
		Animation fairy;
	} textures;
	
	int game_over;
	
	int time;
	int fps;
	
	int lasttime;
} Global;

extern Global global;

void init_global();
void game_over();

void frame_rate();

#endif
