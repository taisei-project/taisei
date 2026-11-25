/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "coroutine/coevent.h"
#include "coroutine/cotask.h"
#include "dialog.h"
#include "enemy.h"
#include "item.h"
#include "lasers/laser.h"
#include "projectile.h"
#include "replay/state.h"
#include "stageinfo.h"

typedef enum GameoverType {
	GAMEOVER_NONE,
	GAMEOVER_DEFEAT = 1,
	GAMEOVER_WIN,
	GAMEOVER_ABORT,
	GAMEOVER_RESTART,
	GAMEOVER_SCORESCREEN = -1,
	GAMEOVER_TRANSITIONING = -2,
} GameoverType;

typedef struct StagePrivateState StagePrivateState;

typedef struct StageInstance {
	StagePrivateState *priv;  // opaque, owned by stage.c
	StageInfo *info;

	COEVENTS_ARRAY(
		end
	) events;

	ProjectileList projs;
	ProjectileList particles;
	EnemyList enemies;
	ItemList items;
	LaserList lasers;

	Boss *boss;
	Dialog *dialog;
	int8_t diff; // Difficulty, but signed to prevent obscure overflow errors
	uint voltage_threshold;
	float view_shake;
	GameoverType gameover;
} StageInstance;

typedef struct GameSession {
	StageInstance stage;

	struct {
		ReplayState input, output;
	} replay;

	GameoverType gameover;
	int8_t diff; // Difficulty, but signed to prevent obscure overflow errors
	bool is_practice_mode;
} GameSession;

// These are only valid in a coroutine task context
#define GAME              (*(GameSession*)NOT_NULL(cotask_get_context(cotask_active_unsafe())))
#define STAGE             (GAME.stage)

static inline bool game_is_demo_mode(const GameSession *game) {
	return game->replay.input.replay && game->replay.input.play.demo_mode;
}
