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
#include "player.h"
#include "projectile.h"
#include "plrmodes.h"
#include "replay/state.h"
#include "resource/resource.h"
#include "stageinfo.h"
#include "util/callchain.h"

typedef enum GameoverType {
	GAMEOVER_NONE,
	GAMEOVER_DEFEAT = 1,
	GAMEOVER_WIN,
	GAMEOVER_ABORT,
	GAMEOVER_RESTART_GAME,
	GAMEOVER_RESTART_STAGE,
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
	int frames;
	int8_t diff; // Difficulty, but signed to prevent obscure overflow errors
	uint voltage_threshold;
	float view_shake;
	GameoverType gameover;
} StageInstance;

typedef struct GameSession {
	Player plr;
	StageInstance stage;
	StageInfo *init_stage;
	CallChain next;
	ResourceGroup rg;

	struct {
		ReplayState input, output;
		int first_stage_idx;
		ReplayPlayFlags play_flags;
	} replay;

	GameoverType gameover;
	int8_t diff; // Difficulty, but signed to prevent obscure overflow errors
	bool is_practice_mode;
} GameSession;

typedef struct GameSettings {
	StageInfo *init_stage;
	Difficulty difficulty;
	CharacterID player_char;
	ShotModeID player_shot;
	bool practice_mode;
	Replay *replay_out;
} GameSettings;

typedef struct ReplayPlaybackSettings {
	Replay *replay;
	Replay *replay_out;
	int first_stage_idx;
	ReplayPlayFlags flags;
} ReplayPlaybackSettings;

bool game_session_init(GameSession *game, const GameSettings *settings)
	attr_nonnull_all;

bool game_session_init_replay(GameSession *game, const ReplayPlaybackSettings *settings)
	attr_nonnull_all;

// Callchain result is the GameSession*
void game_session_enter(GameSession *game, CallChain next)
	attr_nonnull(1);

void game_session_deinit(GameSession *game)
	attr_nonnull_all;

// These are only valid in a coroutine task context
#define GAME   (*(GameSession*)NOT_NULL(cotask_get_context(cotask_active_unsafe())))
#define STAGE  (GAME.stage)

INLINE bool game_is_replay_playback(const GameSession *game) {
	return game->replay.input.replay;
}

INLINE bool game_is_replay_verification(const GameSession *game) {
	return game->replay.input.replay && (game->replay.play_flags & REPLAY_PLAY_VERIFY);
}

INLINE bool game_is_demo_mode(const GameSession *game) {
	return game->replay.input.replay && game->replay.input.play.demo_mode;
}

INLINE bool game_is_story_mode(const GameSession *game) {
	return !game->is_practice_mode && game->stage.info->type == STAGE_STORY;
}
