/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
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

void replay_stage_update_final_stats(ReplayStage *stg, const Stats *stats)
	attr_nonnull_all;

void replay_stage_destroy_events(ReplayStage *stg)
	attr_nonnull_all;
