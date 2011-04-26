/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "audio.h"
#include "global.h"
#include "list.h"

Sound *load_sound(char *filename) {
	ALuint sound;
	if(!(sound = alutCreateBufferFromFile(filename)))
		errx(-1,"load_sound():\n!- cannot load '%s': %s", filename, alutGetErrorString(alutGetError()));
	
	
	Sound *snd = create_element((void **)&global.sounds, sizeof(Sound));
	
	snd->alsnd = sound;
	
	char *beg = strstr(filename, "sfx/") + 4;
	char *end = strrchr(filename, '.');
	
	snd->name = malloc(end - beg + 1);
	memset(snd->name, 0, end-beg + 1);
	strncpy(snd->name, beg, end-beg);
	
	printf("-- loaded '%s' as '%s'\n", filename, snd->name);
	
	return snd;
}

Sound *get_snd(char *name) {
	Sound *s, *res = NULL;
	for(s = global.sounds; s; s = s->next) {
		if(strcmp(s->name, name) == 0)
			res = s;
	}
	
	if(res == NULL)
		errx(-1,"get_snd():\n!- cannot load sound '%s'", name);
	
	return res;
}

void play_sound(char *name) {
	play_sound_p(get_snd(name));
}

void play_sound_p(Sound *snd) {
	ALuint i,res = -1;
	ALint play;
	for(i = 0; i < SNDSRC_COUNT; i++) {
		alGetSourcei(global.sndsrc[i],AL_SOURCE_STATE,&play);
		if(play != AL_PLAYING) {
			res = i;
			break;
		}
	}
	
	if(res != -1) {
		alSourcei(global.sndsrc[res],AL_BUFFER, snd->alsnd);
		alSourcePlay(global.sndsrc[res]);
	} else {
		fprintf(stderr,"play_sound_p():\n!- not enough sources");
	}
}

void delete_sound(void **snds, void *snd) {
	free(((Sound *)snd)->name);
	alDeleteBuffers(1, &((Sound *)snd)->alsnd);
	delete_element(snds, snd);
}

void delete_sounds() {
	delete_all_elements((void **)&global.sounds, delete_sound);
}