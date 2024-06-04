/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "cli.h"
#include "config.h"
#include "dialog.h"
#include "enemy.h"
#include "framerate.h"
#include "item.h"
#include "lasers/laser.h"
#include "projectile.h"
#include "random.h"
#include "replay/state.h"
#include "stageinfo.h"

#include <SDL.h>
#include <SDL_platform.h>

enum {
	// defaults
	RESX = 800,
	RESY = 600,

	VIEWPORT_X = 40,
	VIEWPORT_Y = 20,
	VIEWPORT_W = 480,
	VIEWPORT_H = 560,

	MAX_CONTINUES = 5,

	EVENT_DEATH = -8999,
	EVENT_BIRTH,

	FPS = 60,

	GAMEOVER_SCORE_DELAY = 60,
};

#define VIEWPORT_OFFSET { VIEWPORT_X, VIEWPORT_Y }
#define VIEWPORT_SIZE { VIEWPORT_W, VIEWPORT_H }
#define VIEWPORT_RECT { VIEWPORT_X, VIEWPORT_Y, VIEWPORT_W, VIEWPORT_H }

typedef enum GameoverType {
	GAMEOVER_NONE,
	GAMEOVER_DEFEAT = 1,
	GAMEOVER_WIN,
	GAMEOVER_ABORT,
	GAMEOVER_RESTART,
	GAMEOVER_SCORESCREEN = -1,
	GAMEOVER_TRANSITIONING = -2,
} GameoverType;

typedef struct {
	int8_t diff; // this holds values of type Difficulty, but should be signed to prevent obscure overflow errors
	Player plr;

	ProjectileList projs;
	ProjectileList particles;
	EnemyList enemies;
	ItemList items;
	LaserList lasers;

	int frames; // stage global timer
	int stage_start_frame;

	int frameskip;

	Boss *boss;
	Dialog *dialog;

	GameoverType gameover;
	int gameover_time;

	struct {
		FPSCounter logic;
		FPSCounter render;
		FPSCounter busy;
	} fps;

	struct {
		ReplayState input, output;
	} replay;

	uint voltage_threshold;

	RandomState rand_game;
	RandomState rand_visual;

	StageInfo *stage;

	uint is_practice_mode : 1;
	uint is_headless : 1;
	uint is_replay_verification : 1;
	uint is_kiosk_mode : 1;
} Global;

extern Global global;

void init_global(CLIAction *cli);

void taisei_quit(void);
bool taisei_quit_requested(void);
bool taisei_is_quit_hidden(void);
void taisei_commit_persistent_data(void);

// XXX: Move this somewhere?
bool gamekeypressed(KeyIndex key);
