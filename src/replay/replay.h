/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "util/callchain.h"

#include <SDL.h>

#define REPLAY_EXTENSION "tsr"

#ifdef DEBUG
// #define REPLAY_LOAD_DEBUG
#endif

typedef struct Replay Replay;
typedef struct ReplayStage ReplayStage;
typedef struct ReplayEvent ReplayEvent;

typedef enum ReplayReadMode {
	// bitflags
	REPLAY_READ_META = (1 << 0),
	REPLAY_READ_EVENTS = (1 << 1),

	REPLAY_READ_ALL = REPLAY_READ_META | REPLAY_READ_EVENTS,
} ReplayReadMode;

void replay_reset(Replay *rpy) attr_nonnull_all;
void replay_destroy_events(Replay *rpy) attr_nonnull_all;

bool replay_write(Replay *rpy, SDL_RWops *file, uint16_t version) attr_nonnull_all;
bool replay_read(Replay *rpy, SDL_RWops *file, ReplayReadMode mode, const char *source) attr_nonnull(1, 2);

bool replay_save(Replay *rpy, const char *name) attr_nonnull_all;
bool replay_load(Replay *rpy, const char *name, ReplayReadMode mode) attr_nonnull_all;
bool replay_load_syspath(Replay *rpy, const char *path, ReplayReadMode mode) attr_nonnull_all;
bool replay_load_vfspath(Replay *rpy, const char *path, ReplayReadMode mode) attr_nonnull_all;

int replay_find_stage_idx(Replay *rpy, uint8_t stageid) attr_nonnull_all;

void replay_play(Replay *rpy, int firstidx, CallChain next) attr_nonnull_all;
