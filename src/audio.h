/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <AL/alut.h>

struct Sound;

typedef struct Sound {
	struct Sound *next;
	struct Sound *prev;
	
	ALuint alsnd;
	char *name;
} Sound;

Sound *load_sound(char *filename);
void play_sound(char *name);
void play_sound_p(Sound *snd);

Sound *get_snd(char *name);
void delete_sounds();

#endif