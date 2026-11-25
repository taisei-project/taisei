/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stageinfo.h"
#include "coroutine/cotask.h"
#include "coroutine/coevent.h"

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

	uint voltage_threshold;
	float view_shake;
	GameoverType gameover;
} StageInstance;

typedef struct GameSession {
	StageInstance stage;
	GameoverType gameover;
	bool is_practice_mode;
} GameSession;

// These are only valid in a coroutine task context
#define GAME              (*(GameSession*)NOT_NULL(cotask_get_context(cotask_active_unsafe())))
#define STAGE             (GAME.stage)
