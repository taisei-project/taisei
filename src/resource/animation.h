/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "sprite.h"

//    The animation sequence format
//
// An animation is made up of a number of sprites and an .ani file which
// contains all the metadata.
//
// The .ani file needs to specify the number of sprites using the @sprite_count
// attribute. Then different animation sequences can be defined.
//
// Animation sequences are chains of sprites that can be replayed in-game. For
// example Cirno can either fly normally or flex while flying. In order for the
// game to understand which frames need to be shown in what order and time delay
// you need to define a sequence for every action.
//
// To define the action "right" of the player flying to the left for example,
// you write into the file
//
//    right = d5 0 1 2 3
//
// Every key in the .ani file not starting with @ corresponds to a sequence.
// The sequence specification itself is a list of frame indices. In the example
// above, the right sequence will cycle frames 0-3. Everything that is not a
// number like d5 in the example is a parameter:
//
//    d<n>   sets the frame delay to n. This means every sprite index given is
//           shown for n ingame frames.
//    m      toggles the mirroring of the following frames.
//    m0,m1  set the absolute mirroring of the following frames.
//
// All parameters are persistent within one sequence spec until you change them.
//
// More examples:
//
//    flap = d5 0 1 2 3 d2 4 5 6 7
//    left = m d5 0 1 2 3
//    alternateleftright = m d5 0 1 2 3 m 0 1 2 3
//
// There are many possibilities to use d<n> to make animations look dynamic (or
// strange)
//
// Some naming conventions:
//
// The resource code does not require you to choose any specific names for your
// sequences, but different parts of the game of course have to at some point.
// The most common convention is calling the standard sequence "main". This is
// the least you need to do for anything using an aniplayer, because the
// aniplayer needs to know a valid starting animation.
//
// If you have a sequence of the sprite going left or right, call it "left" and
// "right". Player and fairy animations do this.
//
// Look at existing files for more examples. Wriggle might be interesting for
// complicated delay and queue trickery.
// The documentation of the AniPlayer struct might also be interesting.

typedef struct AniSequenceFrame {
	uint spriteidx;
	bool mirrored;
} AniSequenceFrame;

typedef struct AniSequence {
	AniSequenceFrame *frames;
	int length;
} AniSequence;

typedef struct Animation {
	Hashtable *sequences;
	
	Sprite **sprites;
	Sprite transformed_sprite; // temporary sprite used to apply animation transformations.
	int sprite_count;
} Animation;

char* animation_path(const char *name);
bool check_animation_path(const char *path);
void* load_animation_begin(const char *filename, uint flags);
void* load_animation_end(void *opaque, const char *filename, uint flags);
void unload_animation(void *vani);

Animation *get_ani(const char *name);
AniSequence *get_ani_sequence(Animation *ani, const char *seqname);

// Returns a sprite for the specified frame from an animation sequence named seqname. 
// CAUTION: this sprite is only valid until the next call to this function.
// 
// The frames correspond 1:1 to real ingame frames, so
//
// 	draw_sprite_p(x, y, animation_get_frame(ani,"fly",global.frames));
//
// already gives you the fully functional animation rendering. You can use
// an AniPlayer instance for queueing.
// 
// Note that seqframe loops (otherwise the example above wouldn’t work).
// 
Sprite *animation_get_frame(Animation *ani, AniSequence *seq, int seqframe);


extern ResourceHandler animation_res_handler;

#define ANI_PATH_PREFIX TEX_PATH_PREFIX
#define ANI_EXTENSION ".ani"
