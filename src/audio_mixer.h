/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

// I needed to add this for supporting loop sounds since Mixer doesn’t remember
// what channel a sound is playing on.

typedef struct {
	Mix_Chunk *ch;
	int loopchan; // channel the sound may be looping on. -1 if not looping
} MixerInternalSound;

