/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

/*
 * This is a helper to play animations. It saves the state of the animation and
 * allows queueing different animation sequences.
 */

#include "resource/animation.h"
#include "stageobjects.h"
#include "list.h"

typedef struct AniSequence AniSequence;
struct AniSequence{
	LIST_INTERFACE(AniSequence);

	int row;
	int duration; // number of frames this sequence will be drawn
	int clock;
	int delay; // after the sequence has played loops times before the next one is started.
	int speed; // overrides ani->speed if > 0

	bool mirrored;
	bool backwards;
};

typedef struct AniPlayer AniPlayer;
struct AniPlayer{
	Animation *ani;
	int clock;

	int stdrow; // this is the row rendered when the queue is empty
	bool mirrored;

	AniSequence *queue;
	int queuesize;
};

void aniplayer_create(AniPlayer *plr, Animation *ani) attr_nonnull(1, 2);
void aniplayer_free(AniPlayer *plr);
void aniplayer_reset(AniPlayer *plr); attr_nonnull(1) // resets to a neutral state with empty queue.

// this returns a representation of the frame that is currently drawn by the aniplayer.
// in multirow animations it is computed as follows:
//
// idx = ani->rows*ani->cols*mirrored+row*ani->cols+col
//
int aniplayer_get_frame(AniPlayer *plr) attr_nonnull(1);
void play_animation_frame(Animation *ani, float x, float y, int frame); // context free version that can be used with aniplayer_get_frame to draw a specific state

AniSequence *aniplayer_queue(AniPlayer *plr, int row, int loops, int delay) attr_nonnull(1); // 0 loops: played one time
AniSequence *aniplayer_queue_pro(AniPlayer *plr, int row, int start, int duration, int delay, int speed) attr_nonnull(1); // self-documenting pro version
void aniplayer_update(AniPlayer *plr) attr_nonnull(1); // makes the inner clocks tick
void aniplayer_play(AniPlayer *plr, float x, float y) attr_nonnull(1);

void play_animation(Animation *ani, float x, float y, int row, bool xflip) attr_nonnull(1); // the old way to draw animations without AniPlayer
