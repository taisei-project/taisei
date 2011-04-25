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
#include "laser.h"
#include "shader.h"

enum {	
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
	Laser *lasers;
	
	int frames;
	
	Texture *textures;
	Animation *animations;	
	Sound *sounds;
	Shader *shaders;
	
	struct {
		GLuint fbo;
		GLuint tex;
		GLuint depth;
		
		int nw,nh;
	} rtt;
	
	Boss *boss;
	
	ALuint sndsrc[SNDSRC_COUNT];
	
	int game_over;
	int points;
		
	int fpstime;  // frame counter
	int fps;
	
	int lasttime;
} Global;

extern Global global;

void init_global();
void init_rtt();
void game_over();

void frame_rate();

#endif