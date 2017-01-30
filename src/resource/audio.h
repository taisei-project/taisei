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

Sound *load_sound(char *filename, const char *type);
Sound *load_sound_or_bgm(char *filename, Sound **dest, const char *res_directory, const char *type);

#define play_sound(name) play_sound_p(name, 0);
#define play_ui_sound(name) play_sound_p(name, 1);

void play_sound_p(char *name, int unconditional);

Sound *get_snd(Sound *source, char *name);

void delete_sound(void **snds, void *snd);
void delete_sounds(void);

void set_sfx_volume(float gain);

int init_sfx(int *argc, char *argv[]);
void shutdown_sfx(void);

int init_alut_if_needed(int *argc, char *argv[]);
void unload_alut_if_needed();
int warn_alut_error(const char *when);

#endif
