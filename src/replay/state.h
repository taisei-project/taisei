/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "replay.h"

typedef enum {
	REPLAY_NONE,
	REPLAY_RECORD,
	REPLAY_PLAY
} ReplayMode;

typedef struct ReplayState {
	Replay *replay;
	ReplayStage *stage;
	ReplayMode mode;

	union {
		struct {
			int pos;
			int fps;
			uint16_t desync_check;
			int desync_check_frame;
			int desync_frame;
			int skip_frames;
			bool demo_mode;
		} play;

		struct {
			char _dummy;
		} record;
	};
} ReplayState;

typedef enum ReplaySyncStatus {
	REPLAY_SYNC_NODATA,
	REPLAY_SYNC_OK,
	REPLAY_SYNC_FAIL,
} ReplaySyncStatus;

typedef void (*ReplayEventFunc)(ReplayEvent*, void*);

void replay_state_init_play(ReplayState *rst, Replay *rpy, ReplayStage *rstage)
	attr_nonnull_all;

void replay_state_init_record(ReplayState *rst, Replay *rpy)
	attr_nonnull_all;

void replay_state_deinit(ReplayState *rst)
	attr_nonnull_all;

ReplaySyncStatus replay_state_check_desync(ReplayState *rst, int time, uint16_t check)
	attr_nonnull_all;

void replay_state_play_advance(ReplayState *rst, int frame, ReplayEventFunc event_callback, void *arg)
	attr_nonnull(1, 3);
