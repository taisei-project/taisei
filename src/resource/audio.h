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
	
	int lastplayframe;
	
	ALuint alsnd;
	char *name;
} Sound;

struct current_bgm_t {
	char *name;
	Sound *data;
};

Sound *load_sound(char *filename);
void play_sound(char *name);
void play_sound_p(Sound *snd);

Sound *load_bgm(char *filename);
void start_bgm(char *name);
void stop_bgm(void);

Sound *get_snd(Sound *source, char *name);
void delete_sounds(void);
void delete_music(void);

#endif
