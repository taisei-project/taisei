/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL.h>
#include <SDL_platform.h>

#include "tscomplex.h"

#include "resource/audio.h"
#include "resource/bgm.h"
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
#include "random.h"
#include "events.h"

#define FILE_PREFIX PREFIX "/share/taisei/"
#define CONFIG_FILE "config"

enum {
	// defaults
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
	BOMB_RECOVERY = 300,
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
	GAMEOVER_ABORT,
	GAMEOVER_REWATCH,
	GAMEOVER_RESTART
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
	double stagebg_fps;
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
	int frameskip;
	
	Boss *boss;
	Dialog *dialog;
	
	RefArray refs;
		
	int game_over;
	int points;
	
	FPSCounter fps;
	
	int nostagebg;
	
	Replay replay;
	int replaymode;
	
	float shake_view;
	
	RandomState rand_game;
	RandomState rand_visual;
} Global;

extern Global global;

void print_state_checksum(void);

void init_global(void);

void frame_rate(int *lasttime);
void calc_fps(FPSCounter *fps);
void set_ortho(void);
void colorfill(float r, float g, float b, float a);
void fade_out(float f);
void take_screenshot(void);

// needed for mingw compatibility:
#undef min
#undef max

// NOTE: do NOT convert these to macros please.
// max(frand(), 0.5);
// min(huge_costly_expression, another_huge_costly_expression)
double min(double, double);
double max(double, double);
double clamp(double, double, double);
double approach(double v, double t, double d);
double psin(double);
int strendswith(char *s, char *e);
char* difficulty_name(Difficulty diff);
void stralloc(char **dest, char *src);
int gamekeypressed(int key);

#define SIGN(x) ((x > 0) - (x < 0))

// this is used by both player and replay code
enum {
	EV_PRESS,
	EV_RELEASE,
	EV_OVER, // replay-only
	EV_AXIS_LR,
	EV_AXIS_UD,
	EV_CHECK_DESYNC, // replay-only
};

#endif
