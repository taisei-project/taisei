/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL/SDL.h>

#include "resource/audio.h"
#include "resource/shader.h"
#include "resource/font.h"
#include "resource/animation.h"

#include "menu/menu.h"

#include "player.h"
#include "projectile.h"
#include "enemy.h"
#include "item.h"
#include "boss.h"
#include "laser.h"
#include "dialog.h"
#include "list.h"
#include "config.h"
#include "fbo.h"
#include "vbo.h"
#include "resource/resource.h"
#include "replay.h"

#define FILE_PREFIX PREFIX "/share/taisei/"
#define CONFIG_FILE "config"

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
	ATTACK_START_DELAY = 40,
	BOMB_RECOVERY = 250,
	DEATHBOMB_TIME = 10,
	DEATH_DELAY = 70,
	
	PLR_MAXPOWER = 4,
	PLR_START_LIVES = 2,
	PLR_START_BOMBS = 3,
	MAX_CONTINUES = 3,	
	
	ACTION_DESTROY,
	
	EVENT_DEATH = -8999,
	EVENT_BIRTH,
	
	FPS = 60,
	
	GAMEOVER_DEFEAT = 1,
	GAMEOVER_WIN,
	GAMEOVER_ABORT
};

typedef enum {
	D_Easy = 1,
	D_Normal,
	D_Hard,
	D_Lunatic
} Difficulty;

typedef struct {
	int fpstime;  // frame counter
	int fps;	
	int show_fps;
} FPSCounter;

typedef struct {
	Difficulty diff;
	Player plr;
		
	Projectile *projs;
	Enemy *enemies;
	Item *items;
	Laser *lasers;
	
	Projectile *particles;
	
	int frames;
	int lasttime; // frame limiter
	int timer;
	
	Boss *boss;
	MenuData *menu;
	Dialog *dialog;
	
	RefArray refs; // for super extra OOP-tardness: references. the cool way.
		
	int game_over;
	int points;
	
	FPSCounter fps;
	
	int nostagebg;	// I don't want the automatic stagebg handling to mess with the config, and I don't want that longass if in more than one place either.
	
	Replay replay;
	int replaymode;
} Global;

extern Global global;

void init_global();
void game_over();

void frame_rate(int *lasttime);
void calc_fps(FPSCounter *fps);
void set_ortho();
void fade_out(float f);

void toggle_fullscreen();
void global_processevent(SDL_Event*);
void take_screenshot();

double frand();

// this is used by both player and replay code
enum {
	EV_PRESS,
	EV_RELEASE,
	EV_OVER		// replay-only
};

#define REPLAY_ASKSAVE (tconfig.intval[SAVE_RPY] == 2 && global.replay.active)

#endif
