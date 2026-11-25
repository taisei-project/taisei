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

typedef struct StagePrivateState StagePrivateState;

typedef struct StageInstance {
	StagePrivateState *priv;  // opaque, owned by stage.c
	StageInfo *info;

	float view_shake;
} StageInstance;

typedef struct GameSession {
	StageInstance stage;
} GameSession;

// These are only valid in a coroutine task context
#define GAME              (*(GameSession*)NOT_NULL(cotask_get_context(cotask_active_unsafe())))
#define STAGE             (GAME.stage)
