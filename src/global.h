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
#include "poweritem.h"
#include "audio.h"
#include "boss.h"

enum {
	RESX = 800,
	RESY = 600,
	
	SCREEN_W = 800,
	SCREEN_H = 600,
	
	VIEWPORT_X = 40,
	VIEWPORT_Y = 20,
	VIEWPORT_W = 480,
	VIEWPORT_H = 560,
	
	POINT_OF_COLLECT = VIEWPORT_H/4,
	
	SNDSRC_COUNT = 30,
	
	FPS = 60
};

#define FILE_PREFIX PREFIX "/share/taisei/"

typedef struct {
	Player plr;	
	Projectile *projs;
	Fairy *fairies;
	Poweritem *poweritems;
	
	int frames;
	
	Texture *textures;
	Animation *animations;	
	Sound *sounds;
	
	Boss *boss;
	
	ALuint sndsrc[SNDSRC_COUNT];
	
	int game_over;
	
	int fpstime;  // frame counter
	int fps;
	
	int lasttime;
} Global;

extern Global global;

void init_global();
void game_over();

void frame_rate();

#endif
