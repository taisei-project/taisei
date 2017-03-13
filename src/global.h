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
#include <stdbool.h>
#include <math.h>

#include "util.h"
#include "color.h"

#include "resource/sfx.h"
#include "resource/bgm.h"
#include "resource/shader.h"
#include "resource/font.h"
#include "resource/animation.h"

#include "menu/menu.h"

#include "taiseigl.h"
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
#include "difficulty.h"
#include "color.h"
#include "audio.h"
#include "rwops/all.h"

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

	PLR_MAXPOWER = 400,
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
	GAMEOVER_RESTART,
	GAMEOVER_TRANSITIONING = -1,
};

typedef struct {
	Difficulty diff;
	Player plr;

	Projectile *projs;
	Enemy *enemies;
	Item *items;
	Laser *lasers;

	Projectile *particles;

	int frames;
	int stageuiframes;
	int lasttime; // frame limiter
	int timer;
	int frameskip;

	Boss *boss;
	Dialog *dialog;

	RefArray refs;

	int game_over;

	FPSCounter fps;

	bool nostagebg;

	Replay replay;
	ReplayMode replaymode;
	ReplayStage *replay_stage;

	float shake_view;

	RandomState rand_game;
	RandomState rand_visual;

	StageInfo *stage;
} Global;

extern Global global;

void init_global(void);

#endif
