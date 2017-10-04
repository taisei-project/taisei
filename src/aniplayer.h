/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

/*
 * This is a helper to play animations. It saves the state of the animation and
 * allows queueing different animation sequences.
 */

#include "resource/animation.h"

typedef struct AniSequence AniSequence;
struct AniSequence{
	struct AniSequence *next;
	struct AniSequence *prev;

	int row;
	int duration; // number of frames this sequence will be drawn
	int clock;
	int delay; // after the sequence has played loops times before the next one is started.
	int speed; // overrides ani->speed if > 0

	bool mirrored;
	bool backwards;
};

typedef struct {
	Animation *ani;
	int clock;

	int stdrow; // this is the row rendered when the queue is empty
	bool mirrored;

	AniSequence *queue;
	int queuesize;
} AniPlayer;

void aniplayer_create(AniPlayer *plr, Animation *ani);
void aniplayer_free(AniPlayer *plr);

void aniplayer_reset(AniPlayer *plr); // resets to a neutral state with empty queue.

AniSequence *aniplayer_queue(AniPlayer *plr, int row, int loops, int delay); // 0 loops: played one time
AniSequence *aniplayer_queue_pro(AniPlayer *plr, int row, int start, int duration, int delay, int speed); // self-documenting pro version
void aniplayer_update(AniPlayer *plr); // makes the inner clocks tick
void aniplayer_play(AniPlayer *plr, float x, float y);

void play_animation(Animation *ani, float x, float y, int row); // the old way to draw animations without AniPlayer
