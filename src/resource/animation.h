/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_animation_h
#define IGUARD_resource_animation_h

#include "taisei.h"

#include "resource.h"
#include "sprite.h"

typedef struct AniSequenceFrame {
	uint spriteidx;
	bool mirrored;
} AniSequenceFrame;

typedef struct AniSequence {
	AniSequenceFrame *frames;
	int length;
} AniSequence;

typedef struct Animation {
	ht_str2ptr_t sequences;
	ResourceRef *sprites;
	Sprite transformed_sprite; // temporary sprite used to apply animation transformations.
	int sprite_count;
} Animation;

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

#define ANI_EXTENSION ".ani"

#endif // IGUARD_resource_animation_h
