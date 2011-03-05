/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
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

#define FILE_PREFIX PREFIX "/share/seiyou/"

#define DT (SDL_GetTicks() - global.time)
#define DTe ((float)DT/16.0)

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
	
	int fpstime;  // frame counter
	int fps;
	
	int lasttime;
} Global;

extern Global global;

void init_global();
void game_over();

void frame_rate();

#endif
