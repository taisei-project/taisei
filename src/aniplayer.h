/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// In principle, playing an animation does not require any state, just a call to
// get_animation_frame will get you the right sprite out of a sequence. But if
// you want easy queueing and seamless switching between animation sequences,
// you can use an AniPlayer.
//
// It works like this: AniPlayer contains a queue where you can enqueue
// animation sequences using aniplayer_queue. Then the player will count as many
// loops/frames as you specified and once done, go to the next queue entry. If
// there is no other queue entry, the current entry is looped until something is
// added. Then the queue will switch at the next sequence endpoint for seamlessness.
//
// For aniplayer_queue takes a number of minimum loops that should be played
// before the animation is switched (remember it goes on forever if there is
// nothing to follow). For convenience there is also aniplayer_queue_frames
// which allows you to specify a minimum number of frames.
//
// If you need to go to a new sequence asap but still in a seamless way you can use
// aniplayer_soft_switch to forget about the queue and do your sequence directly
// after the currently running one.
//
// If that is still not okay for you (You just need to switch instantly, I
// respect that) you can call aniplayer_hard_switch to switch to a new sequence
// directly. This won’t be seamless though. Seamlessness is the biggest quirk of
// this api because it typically delays things and you need to queue early
// enough to get the queue transition at the right time.
//
// Examples:
//
// 1. make Hina spin one loop
//
// 	aniplayer_queue(&h->ani, "guruguru", 1);
// 	aniplayer_queue(&h->ani, "main", 0);
//
// The second line is important. Giving 0 minloops as an argument is common.
// It just means that the animation should not play longer than need be.
// In fact, having a call with minloops != 0 not followed by another call does
// not make much sense. If you want to play n-loops you need to provide
// something to switch to.
//
// 2. make Cirno flex for 30 frames
//
// 	aniplayer_queue_frames(&c->ani, "(9)", 30);
// 	aniplayer_queue(&c->ani, "main", 0);
//
// Same story. This flavor is also useful in some spells.
//
// 3. make Cirno flex for the whole spell
//
//	aniplayer_queue(&c->ani, "(9)", 0);
//
// 4. make the player react to an input instantly
//
//	aniplayer_hard_switch(&plr->ani, "right", 0);
//
// Similar examples occur throughout the code so if you want context, you can just look there.
//
#include "resource/animation.h"
#include "list.h"

typedef struct AniQueueEntry AniQueueEntry;
struct AniQueueEntry{
	LIST_INTERFACE(AniQueueEntry);

	AniSequence *sequence;
	int clock; // frame counter. As long as clock < duration this entry will keep running
	int duration; // number of frames this sequence will be drawn
};

typedef struct AniPlayer AniPlayer;
struct AniPlayer{
	Animation *ani;

	LIST_ANCHOR(AniQueueEntry) queue;
	int queuesize;
};

void aniplayer_create(AniPlayer *plr, Animation *ani, const char *startsequence) attr_nonnull(1, 2);
void aniplayer_free(AniPlayer *plr);

// AniPlayer version of animation_get_frame.
Sprite *aniplayer_get_frame(AniPlayer *plr) attr_nonnull(1);

// See comment above for these three stooges.
AniQueueEntry *aniplayer_queue(AniPlayer *plr, const char *seqname, int minloops) attr_nonnull(1);
AniQueueEntry *aniplayer_soft_switch(AniPlayer *plr, const char *seqname, int minloops) attr_nonnull(1);
AniQueueEntry *aniplayer_hard_switch(AniPlayer *plr, const char *seqname, int minloops) attr_nonnull(1);

// This fourth stooge also queues a sequence but for at least the given amount of frames
// Useful in the stage code.
AniQueueEntry *aniplayer_queue_frames(AniPlayer *plr, const char *seqname, int minframes) attr_nonnull(1);

// AniPlayers need to be actively updated in order to tick (unlike most of
// the rest of the game which just uses global.frames as a counter). So you
// need to call this function once per frame to make an animation move.
void aniplayer_update(AniPlayer *plr) attr_nonnull(1);
