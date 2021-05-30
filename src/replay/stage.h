/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_replay_stage_h
#define IGUARD_replay_stage_h

#include "taisei.h"

#include "replay.h"
#include "player.h"
#include "stageinfo.h"
#include "difficulty.h"

ReplayStage *replay_stage_new(
	Replay *rpy,
	StageInfo *stage,
	uint64_t start_time,
	uint64_t seed,
	Difficulty diff,
	Player *plr
) attr_nonnull_all attr_returns_nonnull;

void replay_stage_event(ReplayStage *stg, uint32_t frame, uint8_t type, uint16_t value)
	attr_nonnull_all;

void replay_stage_sync_player_state(ReplayStage *stg, Player *plr)
	attr_nonnull_all;

#endif // IGUARD_replay_stage_h
