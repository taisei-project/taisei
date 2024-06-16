/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "sprite.h"

typedef struct AniSequence {
	int length;
	int frame_indices[];
} AniSequence;

typedef struct Animation {
	ht_str2ptr_t sequences;
	Sprite **sprites;
	Sprite *local_sprites;
	int sprite_count;
} Animation;

DEFINE_RESOURCE_GETTER(Animation, res_anim, RES_ANIM)
DEFINE_OPTIONAL_RESOURCE_GETTER(Animation, res_anim_optional, RES_ANIM)

AniSequence *get_ani_sequence(Animation *ani, const char *seqname);

// Returns a sprite for the specified frame from an animation sequence named seqname.
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
