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
#include "cli.h"

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
	ATTACK_START_DELAY = 60,
	ATTACK_START_DELAY_EXTRA = 150,
	ATTACK_END_DELAY = 20,
	ATTACK_END_DELAY_EXTRA = 150,
	BOSS_DEATH_DELAY = 150,
	BOMB_RECOVERY = 300,
	DEATHBOMB_TIME = 12,
	DEATH_DELAY = 70,

	MAX_CONTINUES = 3,

	EVENT_DEATH = -8999,
	EVENT_BIRTH,
	ACTION_DESTROY,

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
	uint64_t lasttime; // frame limiter
	int timer;
	int frameskip;

	Boss *boss;
	Dialog *dialog;

	RefArray refs;

	int game_over;

	FPSCounter fps;

	Replay replay;
	ReplayMode replaymode;
	ReplayStage *replay_stage;

	float shake_view;
	float shake_view_fade;

	RandomState rand_game;
	RandomState rand_visual;

	StageInfo *stage;
	int continues;
} Global;

extern Global global;

void init_global(CLIAction *cli);

#endif
